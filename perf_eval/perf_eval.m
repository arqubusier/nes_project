
close all
clear all
clc

%% Import simulation log
% tbl = import_log('../test_results/scenario_my1.txt');
tbl = import_log('../test_results/scenario2.txt');

%% Powertrace
% Extract powertrace information from output messages
% tbl = get_powertrace(tbl);

% Plot powertrace data
% plot_powertrace(tbl);

%% Packet delivery
% Print the list of sent and received packets
% print_packetdelivery(tbl);

%% Misc
% Get the min/avg/max values of transmission times
print_trtime(tbl);