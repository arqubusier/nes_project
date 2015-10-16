function plot_powertrace(tbl)

plot all cpu usage for the first relay node
idx = tbl.ID == relayNodeList(1) & tbl.MsgType == 'P';

figure(1)
plot(tbl.TimeStamp(idx), tbl.cpu(idx), '*-');
title('CPU')
xlabel('time [s]')

figure(2)
plot(tbl.TimeStamp(idx), tbl.transmit(idx), '*-');
title('Transmission')
xlabel('time [s]')

figure(3)
plot(tbl.TimeStamp(idx), tbl.listen(idx), '*-');
title('Listening')
xlabel('time [s]')