function print_results(tbl, lat_tbl)
% Calculate the average Latency and PDR

    %% Latency
    lat_tbl.Latency.Format = 'mm:ss.SSS';
    fprintf('Min latency: %s\n', char(min(lat_tbl.Latency)));
    fprintf('Max latency: %s\n', char(max(lat_tbl.Latency)));
    fprintf('Average latency: %s\n', char(mean(lat_tbl.Latency(~isnan(lat_tbl.Latency)))));
    fprintf('STD latency: %.3f seconds\n', std(seconds(lat_tbl.Latency(~isnan(lat_tbl.Latency)))));

    valid_lat = ~isnan(lat_tbl.Latency);

    min_proc3 = 100 * (sum(lat_tbl.Latency(valid_lat) < minutes(2)) / length(lat_tbl.Latency(valid_lat)));
    fprintf('Under 2 min: %.1f%%\n', min_proc3);

    %% Packet delivery ratio

    % not counting the packets generated in the last minute
    last_timestamp = tbl.TimeStamp(end) - tbl.TimeStamp(1);

    % a sensor sample is created in every 12 seconds, which means one sensor
    % packet in every minute
    nr_packets = floor(minutes(last_timestamp));

    fprintf('\nApprox nr of gen sensor packets: %d /node\n', nr_packets);
    fprintf('Packet delivery for each sensor node:\n');

    sn_list = unique(lat_tbl.SNid);
    proc_all = 0;
    for i = 1 : length(sn_list)
        nr_delpacket = sum(lat_tbl.SNid(valid_lat) == sn_list(i));
        proc = 100*nr_delpacket/nr_packets;
        proc_all = proc_all + proc;
        fprintf('\t%d: %d (%.1f%%)\n', sn_list(i), nr_delpacket, proc);
    end
    fprintf('Average packet delivery: %.2f%%\n', proc_all/length(sn_list));
