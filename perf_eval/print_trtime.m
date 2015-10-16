% function print_trtime(tbl)

% relay node ack messages sent to sensor nodes
rn_ack_tbl = tbl(tbl.NodeType == 'RN' & tbl.MsgType == 'S' & tbl.PktType == 'SAC', :);

C = cellfun(@(x) textscan(char(x),'RN_S_SAC_ADDR_%d_SQN_%d'), ...
        rn_ack_tbl.Output, 'UniformOutput', false);

rn_ack_tbl.DestSN = cell2mat(cellfun(@(x) x{1}, C, 'UniformOutput', false));
rn_ack_tbl.SQN = cell2mat(cellfun(@(x) x{2}, C, 'UniformOutput', false));

% sensor node received ack messages
sn_ack_tbl = tbl(tbl.NodeType == 'SN' & tbl.MsgType == 'R' & tbl.PktType == 'SAC', :);

C = cellfun(@(x) textscan(char(x),'SN_R_SAC_SQN_%d'), ...
        sn_ack_tbl.Output, 'UniformOutput', false);

sn_ack_tbl.SQN = cell2mat(cellfun(@(x) x{1}, C, 'UniformOutput', false));

time_diff = [];

for i = 1:height(rn_ack_tbl)
    sn_idx = find(sn_ack_tbl.ID == rn_ack_tbl.DestSN(i) & ...
        sn_ack_tbl.SQN == rn_ack_tbl.SQN(i) & ...
        sn_ack_tbl.TimeStamp > rn_ack_tbl.TimeStamp(i), ...
        1);
    
    exist_newer = sum(rn_ack_tbl.DestSN(i:end) == rn_ack_tbl.DestSN(i) & rn_ack_tbl.SQN(i:end) == rn_ack_tbl.SQN(i));
    
    if sum(sn_ack_tbl.ID == rn_ack_tbl.DestSN(i) & sn_ack_tbl.SQN == rn_ack_tbl.SQN(i) & sn_ack_tbl.TimeStamp > rn_ack_tbl.TimeStamp(i)) > 1
        i
    end
    
    if exist_newer <= 1
        time_diff = [time_diff; sn_ack_tbl.TimeStamp(sn_idx) - rn_ack_tbl.TimeStamp(i)];
    end
end

time_diff.Format = 'mm:ss.SSS';

min(time_diff)
mean(time_diff)
max(time_diff)