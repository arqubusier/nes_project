function tbl = import_log(filename)

% tbl = readtable('test_results/scenario_my1.txt', 'ReadVariableNames', false, 'Delimiter', '\t');
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
    regexp(tbl.Output, '[(BS)(RN)(SN)]_[EPRS]_'), ...
    'UniformOutput', false)));

% Node type: BS - base station; RN - relay node; SN - sensor node
node_type = repmat({'UNSPECIFIED'}, height(tbl), 1);

node_type(tbl.EVAL_MSG) = cellfun(@(x) x(1:2), tbl.Output(tbl.EVAL_MSG), 'UniformOutput', false);
tbl.NodeType = categorical(node_type);
clear node_type

baseStationList = unique(tbl.ID(tbl.NodeType == 'BS'));
relayNodeList = unique(tbl.ID(tbl.NodeType == 'RN'));
sensorNodeList = unique(tbl.ID(tbl.NodeType == 'SN'));

% Message type: S - sent; R - received; P - powertrace; E - extracted
% sensor packet from aggregated packet
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
