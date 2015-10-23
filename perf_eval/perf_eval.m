
close all
clear all
clc

%% Import simulation log
% tbl = import_log('../test_results/scenario_my1.txt');
% tbl = import_log('../test_results/scenario2.txt');
tbl = import_log('../test_results/22oct_r6_s30_20min_PT.txt');

%% Powertrace
% Extract powertrace information from output messages
pow_tbl = get_powertrace(tbl);

% Plot powertrace data
% plot_powertrace(pow_tbl);

%% Packet delivery and latency
% Print the list of sent and received packets
print = false;
lat_tbl = print_packetdelivery(tbl, print);
plot_latency(tbl, lat_tbl);

%% Misc
% Get the min/avg/max values of transmission times
% print_trtime(tbl);

