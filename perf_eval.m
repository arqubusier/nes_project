function perf_eval(filename)

% tbl = readtable('loglistener5_complex.txt', 'ReadVariableNames', false, 'Delimiter', '\t');
try
    tbl = readtable(filename, 'ReadVariableNames', false, 'Delimiter', '\t');
catch
    fprintf('Wrong file name, or file not in this folder!\n');
    return
end

tbl.Properties.VariableNames{'Var1'} = 'TimeStr';
tbl.Properties.VariableNames{'Var2'} = 'IDStr';
tbl.Properties.VariableNames{'Var3'} = 'Output';

% Extract DateTime from str
tbl.TimeStamp = datetime(tbl.TimeStr, 'InputFormat', 'mm:ss.SSS');
tbl.TimeStamp.Format = 'mm:ss.SSS';

% Extract ID from str
a = char(tbl.IDStr);
tbl.ID = str2double(cellstr(a(:, 4:end)));
clear a

% The Output is a message intended for performance evaluatiion
tbl.EVAL_MSG = logical(cell2mat(cellfun(@(x) ~isempty(x), ... % && x == 2, ...
    regexp(tbl.Output, '[(BS)(RN)(SN)]_[PRS]_'), ...
    'UniformOutput', false)));

% Node type: BS - base station; RN - relay node; SN - sensor node
node_type = repmat({'UNSPECIFIED'}, height(tbl), 1);

node_type(tbl.EVAL_MSG) = cellfun(@(x) x(1:2), tbl.Output(tbl.EVAL_MSG), 'UniformOutput', false);
tbl.NodeType = categorical(node_type);
clear node_type

baseStationList = unique(tbl.ID(tbl.NodeType == 'BS'));
relayNodeList = unique(tbl.ID(tbl.NodeType == 'RN'));
sensorNodeList = unique(tbl.ID(tbl.NodeType == 'SN'));

% Message type: S - sent; R - received; P - powertrace
msg_type = repmat({'UNSPECIFIED'}, height(tbl), 1);

msg_type(tbl.EVAL_MSG) = cellfun(@(x) x(4), tbl.Output(tbl.EVAL_MSG), 'UniformOutput', false);
tbl.MsgType = categorical(msg_type);
clear msg_type

% Packet type: ACK - acknowledge for agg data, DAT - agg data, CON -
% config, SPA - sesor packet, SAC - sensor ack
packet_type = repmat({'UNSPECIFIED'}, height(tbl), 1);

packet_type(tbl.EVAL_MSG) = cellfun(@(x) x(6:8), tbl.Output(tbl.EVAL_MSG), 'UniformOutput', false);
tbl.PktType = categorical(packet_type);
clear packet_type

% powertrace output format:
% str, 1 -> clock_time(), P 2-> ( rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1] ), 
% 3-> seqno, 4-> all_cpu, 5-> all_lpm, 6-> all_transmit, 7-> all_listen, 8-> all_idle_transmit, 9-> all_idle_listen, 
% 10-> cpu, 11-> lpm, 12-> transmit, 13-> listen, idle_transmit, idle_listen, followed by some mathematical numbers..
tbl.cpu = zeros(height(tbl), 1);
tbl.transmit = zeros(height(tbl), 1);
tbl.listen = zeros(height(tbl), 1);

C = cellfun(@(x) textscan(char(x),'%*2c_P_ %d P %d %d %d %d %d %d %d %d %d %d %d %d %*[^\n]'), ...
        tbl.Output(tbl.MsgType == 'P'), 'UniformOutput', false);

% cpu - 4
tbl.cpu(tbl.MsgType == 'P') = cell2mat(cellfun(@(x) x{10}, C, 'UniformOutput', false));
tbl.transmit(tbl.MsgType == 'P') = cell2mat(cellfun(@(x) x{12}, C, 'UniformOutput', false));
tbl.listen(tbl.MsgType == 'P') = cell2mat(cellfun(@(x) x{13}, C, 'UniformOutput', false));

% plot all cpu usage for the first relay node
% idx = tbl.ID == relayNodeList(1) & tbl.MsgType == 'P';
%
% figure(1)
% plot(tbl.TimeStamp(idx), tbl.cpu(idx), '*-');
% title('CPU')
% xlabel('time [s]')
% 
% figure(2)
% plot(tbl.TimeStamp(idx), tbl.transmit(idx), '*-');
% title('Transmission')
% xlabel('time [s]')
% 
% figure(3)
% plot(tbl.TimeStamp(idx), tbl.listen(idx), '*-');
% title('Listening')
% xlabel('time [s]')

%% Packet delivery

bs_tbl = tbl(tbl.NodeType == 'BS' & tbl.MsgType == 'R' & tbl.PktType == 'DAT', :);
sn_tbl = tbl(tbl.NodeType == 'SN' & tbl.MsgType == 'S' & tbl.PktType == 'SPA', :);

C = cellfun(@(x) textscan(char(x),'SN_S_SPA_ADDR_%d_SQN_%d_DATA_%d %d %d %d %d %d %d %d %d '), ...
        sn_tbl.Output, 'UniformOutput', false);
op1 = cell2mat(cellfun(@(x) [x{1} x{2} x{3} x{4} x{5} x{6} x{7} x{8} x{9} x{10} x{11}], C, 'UniformOutput', false));
op1 = [(1:length(op1))' op1];

C = cellfun(@(x) textscan(char(x),'BS_R_DAT_ADDR_%d_SQN_%d_DATA_%d %d %d %d %d %d %d %d %d '), ...
        bs_tbl.Output, 'UniformOutput', false);
op2 = cell2mat(cellfun(@(x) [x{1} x{2} x{3} x{4} x{5} x{6} x{7} x{8} x{9} x{10} x{11}], C, 'UniformOutput', false));
op2 = [(1:length(op2))' op2];

%%
sn_list = unique(op1(:,2));

for i = 1:length(sn_list)
    node_pkt_list = op1(op1(:,2) == sn_list(i), :);
    fprintf('SN: %d\n', sn_list(i));
    
    sqn_list = unique(node_pkt_list(:,3));
    
    for j = 1:length(sqn_list)
        sqn_pkt_list = node_pkt_list(node_pkt_list(:,3) == sqn_list(j), :);
        
        fprintf('\tSQNO: %d \tfs: %s, nrs: %d, ', sqn_list(j), char(sn_tbl.TimeStr(sqn_pkt_list(1,1))), size(sqn_pkt_list, 1));
        
        pkt_ids = find(ismember(op2(:, 2:end), sqn_pkt_list(:, 2:end), 'rows'));
        if isempty(pkt_ids)
            fprintf('PACKET NOT RECEIVED!\n');
        else
            fprintf('fr: %s, nrr: %d\n', char(bs_tbl.TimeStr(op2(pkt_ids(1),1))), length(pkt_ids));
        end
        
    end
end