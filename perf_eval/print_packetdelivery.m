function print_packetdelivery(tbl)

bs_tbl = tbl(tbl.NodeType == 'BS' & tbl.MsgType == 'R' & tbl.PktType == 'DAT', :);
sn_tbl = tbl(tbl.NodeType == 'SN' & tbl.MsgType == 'S' & tbl.PktType == 'SPA', :);

C = cellfun(@(x) textscan(char(x),'SN_S_SPA_ADDR_%d_SQN_%d_DATA_%d %d %d %d %d %d %d %d %d '), ...
        sn_tbl.Output, 'UniformOutput', false);
op1 = cell2mat(cellfun(@(x) [x{1} x{2} x{3} x{4} x{5} x{6} x{7} x{8} x{9} x{10} x{11}], C, 'UniformOutput', false));
op1 = [(1:size(op1, 1))' op1];

C = cellfun(@(x) textscan(char(x),'BS_E_SPA_ADDR_%d_SQN_%d_DATA_%d %d %d %d %d %d %d %d %d '), ...
        bs_tbl.Output, 'UniformOutput', false);
op2 = cell2mat(cellfun(@(x) [x{1} x{2} x{3} x{4} x{5} x{6} x{7} x{8} x{9} x{10} x{11}], C, 'UniformOutput', false));
op2 = [(1:size(op2, 1))' op2];

% Print out the sent and received packets
sn_list = unique(op1(:,2));

for i = 1:length(sn_list)
    node_pkt_list = op1(op1(:,2) == sn_list(i), :);
    fprintf('SN: %d\n', sn_list(i));
    
    sqn_list = unique(node_pkt_list(:,3));
    
    for j = 1:length(sqn_list)
        sqn_pkt_list = node_pkt_list(node_pkt_list(:,3) == sqn_list(j), :);
        
        fprintf('\tSQNO: %d \tls: %s, nrs: %d, ', sqn_list(j), char(sn_tbl.TimeStr(sqn_pkt_list(end,1))), size(sqn_pkt_list, 1));
        
        pkt_ids = find(ismember(op2(:, 2:end), sqn_pkt_list(:, 2:end), 'rows'));
        if isempty(pkt_ids)
            fprintf('PACKET NOT RECEIVED!\n');
        else
            fprintf('fr: %s, nrr: %d\n', char(bs_tbl.TimeStr(op2(pkt_ids(1),1))), length(pkt_ids));
        end
        
    end
end