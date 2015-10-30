function plot_powertrace(tbl)

tbl.TimeStamp.Format = 'mm:ss.SSS';

% plot all cpu usage for sensor node 1
idx = tbl.ID == 1;

figure(1)
ax(1) = subplot(4,1,1);
plot(tbl.TimeStamp(idx), tbl.p_cpu(idx), '*-');
title('Power [mW]')
ylabel('CPU')

ax(2) = subplot(4,1,2);
plot(tbl.TimeStamp(idx), tbl.p_lpm(idx), '*-');
ylabel('LPM')

ax(3) = subplot(4,1,3);
plot(tbl.TimeStamp(idx), tbl.p_tx(idx), '*-');
ylabel('TX')

ax(4) = subplot(4,1,4);
plot(tbl.TimeStamp(idx), tbl.p_rx(idx), '*-');
ylabel('RX')
xlabel('Time [s]')

linkaxes(ax, 'x')