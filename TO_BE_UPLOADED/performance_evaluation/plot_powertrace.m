function plot_powertrace(tbl, nodeID)
% Plot the current consumption of a specified node, and calculate the
% expected lifetime

    tbl.TimeStamp.Format = 'mm:ss.SSS';

    idx = tbl.ID == nodeID;
    
    % Use data from between 2 end 18 minutes of simulation
    timeS = min(tbl.TimeStamp);
    timeS1 = timeS(1) + minutes(2);
    timeS2 = timeS(1) + minutes(18);

    idxTime = false(size(idx));
    idxTime(idx) = tbl.TimeStamp(idx) >= timeS1 & tbl.TimeStamp(idx) <= timeS2;

    % Calculate the total current consumption
    power = tbl.p_cpu(idxTime) + tbl.p_lpm(idxTime) + tbl.p_tx(idxTime) + tbl.p_rx(idxTime);
    
    % Calculate the expected lifetime assuming 2 AA batteries with 2500 mAh capacity
    dayNr = (2500/(mean(power)))/24;
    
    fprintf('Average current consumption: %.2f mA\n', mean(power));
    fprintf('Expected lifetime: %.1f days assuming 2xAA 2500mAh batteries\n', dayNr);


    % Plot the current consumption of the different modules
    figure(1)
    ax(1) = subplot(4,1,1);
    plot(tbl.TimeStamp(idx), tbl.p_cpu(idx), '*-');
    title('Current [mA]')
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