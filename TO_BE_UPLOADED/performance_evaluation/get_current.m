function [Amin, Amax, Amean] = get_current(tbl, type)
% Calculate the min, max and avg current consumption for NodeType 'type'

list = unique(tbl.ID(tbl.NodeType == type));

Aval = [];

for idN = 1:length(list)
    idx = tbl.ID == list(idN);

    timeS = min(tbl.TimeStamp);
    timeS1 = timeS(1) + minutes(2);
    timeS2 = timeS(1) + minutes(18);

    idxTime = false(size(idx));
    idxTime(idx) = tbl.TimeStamp(idx) >= timeS1 & tbl.TimeStamp(idx) <= timeS2;

    current = tbl.p_cpu(idxTime) + tbl.p_lpm(idxTime) + tbl.p_tx(idxTime) + tbl.p_rx(idxTime);
    Aval = [Aval; mean(current)];
end

Amin = min(Aval);
Amax = max(Aval);
Amean = mean(Aval);