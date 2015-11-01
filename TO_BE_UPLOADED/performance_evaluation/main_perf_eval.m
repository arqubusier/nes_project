% MATLAB script created to evaluate the performace of the implemented and
% simulated cow monitoring WSN

%% Import simulation log
tbl = import_log('../../test_results_final/r6s15.txt');

%% Powertrace
% Extract powertrace information from output messages
pow_tbl = get_powertrace(tbl);

% Plot powertrace data
plot_powertrace(pow_tbl, 10);

%% Packet delivery and latency
% Print the list of sent and received packets
print_list = false;
lat_tbl = print_packetdelivery(tbl, print_list);
print_results(tbl, lat_tbl);

