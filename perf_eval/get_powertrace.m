function tbl = get_powertrace(tbl)

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