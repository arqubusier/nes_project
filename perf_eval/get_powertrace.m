function tbl = get_powertrace(tbl)

tbl = tbl(tbl.MsgType == 'P', :);

% powertrace output format:
% str, 1 -> clock_time(), P 2-> ( rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1] ), 
% 3-> seqno, 4-> all_cpu, 5-> all_lpm, 6-> all_transmit, 7-> all_listen, 8-> all_idle_transmit, 9-> all_idle_listen, 
% 10-> cpu, 11-> lpm, 12-> transmit, 13-> listen, idle_transmit, idle_listen, followed by some mathematical numbers..

C = cellfun(@(x) textscan(char(x),'%*2c_P_ %d P %d %d %d %d %d %d %d %d %d %d %d %d %*[^\n]'), ...
        tbl.Output, 'UniformOutput', false);

tbl.cpu = double(cell2mat(cellfun(@(x) x{10}, C, 'UniformOutput', false)));
tbl.lpm = double(cell2mat(cellfun(@(x) x{11}, C, 'UniformOutput', false)));
tbl.tx = double(cell2mat(cellfun(@(x) x{12}, C, 'UniformOutput', false)));
tbl.rx = double(cell2mat(cellfun(@(x) x{13}, C, 'UniformOutput', false)));

% calculation info from: http://sourceforge.net/p/contiki/mailman/contiki-developers/thread/CAGveKHgLBivtvYFZAaX2gD2APwtC_MfYZqoauC86Ys1QB54Ynw%40mail.gmail.com/

tm = tbl.cpu + tbl.lpm; % time

% Values from datasheet: http://www.eecs.harvard.edu/~konrad/projects/shimmer/references/tmote-sky-datasheet.pdf
% Power calculated in mW
tbl.p_cpu = tbl.cpu * 1.8 ./ tm;
tbl.p_lpm = tbl.lpm * 0.545 ./ tm;
tbl.p_tx = tbl.tx * 17.7 ./ tm;
tbl.p_rx = tbl.rx * 20 ./ tm;