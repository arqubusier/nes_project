function packet_tbl = print_packetdelivery(tbl, print)
% Calculate and print the packet delivery for each node and each packet
    
    % base station - received sensor packets
    bs_tbl = tbl(tbl.NodeType == 'BS' & tbl.MsgType == 'E' & tbl.PktType == 'SPA', :);
    % sensor node - sent sensor packets
    sn_tbl = tbl(tbl.NodeType == 'SN' & tbl.MsgType == 'S' & tbl.PktType == 'SPA', :);

    C = cellfun(@(x) textscan(char(x),'SN_S_SPA_ADDR_%d_SQN_%d_DATA_%d %d %d %d %d %d %d %d %d '), ...
            sn_tbl.Output, 'UniformOutput', false);
    op1 = cell2mat(cellfun(@(x) [x{1} x{2} x{3} x{4} x{5} x{6} x{7} x{8} x{9} x{10} x{11}], C, 'UniformOutput', false));
    op1 = [(1:size(op1, 1))' op1];

    C = cellfun(@(x) textscan(char(x),'BS_E_SPA_ADDR_%d_SQN_%d_DATA_%d %d %d %d %d %d %d %d %d '), ...
            bs_tbl.Output, 'UniformOutput', false);
    op2 = cell2mat(cellfun(@(x) [x{1} x{2} x{3} x{4} x{5} x{6} x{7} x{8} x{9} x{10} x{11}], C, 'UniformOutput', false));
    op2 = [(1:size(op2, 1))' op2];

    % Table for packet latency
    packet_tbl = table;

    % empty packet_tbl row
    empty_tbl = table;
    empty_tbl.SNid = NaN;
    empty_tbl.SQN = NaN;
    empty_tbl.LastSent = datetime('NaT');
    empty_tbl.FirstRecv = datetime('NaT');
    empty_tbl.Latency = datetime('NaT');

    % Print out the sent and received packets
    sn_list = unique(op1(:,2));

    for i = 1:length(sn_list)
        node_pkt_list = op1(op1(:,2) == sn_list(i), :);
        if print
            fprintf('SN: %d\n', sn_list(i));
        end

        sqn_list = unique(node_pkt_list(:,3));

        for j = 1:length(sqn_list)

            new_row = empty_tbl;
            new_row.SNid = sn_list(i);
            new_row.SQN = sqn_list(j);

            sqn_pkt_list = node_pkt_list(node_pkt_list(:,3) == sqn_list(j), :);

            new_row.LastSent = sn_tbl.TimeStamp(sqn_pkt_list(end,1));
            if print
                fprintf('\tSQNO: %d \tls: %s, nrs: %d, ', sqn_list(j), char(sn_tbl.TimeStr(sqn_pkt_list(end,1))), size(sqn_pkt_list, 1));
            end

            pkt_ids = find(ismember(op2(:, 2:end), sqn_pkt_list(:, 2:end), 'rows'));
            if isempty(pkt_ids)
                if print
                    fprintf('PACKET NOT RECEIVED!\n');
                end
            else
                new_row.FirstRecv = bs_tbl.TimeStamp(op2(pkt_ids(1),1));
                if print
                    fprintf('fr: %s, nrr: %d\n', char(bs_tbl.TimeStr(op2(pkt_ids(1),1))), length(pkt_ids));
                end
            end

            new_row.Latency = new_row.FirstRecv - new_row.LastSent;
            packet_tbl = [packet_tbl; new_row];

        end
    end