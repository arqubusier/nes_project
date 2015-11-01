% Create a table with current consumption statistics of sensor and relay
% nodes

% Create list of available logfiles
file_list = dir('../test_results_final/r*.txt');
file_list = {file_list(:).name}';

tblPower = table;
emptyRow = table;
emptyRow.Name = {''};
emptyRow.RNmin = NaN;
emptyRow.RNmax = NaN;
emptyRow.RNmean = NaN;
emptyRow.RNmeanDay = NaN;
emptyRow.SNmin = NaN;
emptyRow.SNmax = NaN;
emptyRow.SNmean = NaN;
emptyRow.SNmeanDay = NaN;

for idf = 1:length(file_list)
    
    newRow = emptyRow;
    
    newRow.Name = file_list(idf);
    tbl = import_log(['../test_results_final/' file_list{idf}]);
    
    pow_tbl = get_powertrace(tbl);
    
    [newRow.SNmin, newRow.SNmax, newRow.SNmean] = get_current(pow_tbl, 'SN');
    newRow.SNmeanDay = (2500/newRow.SNmean)/24;
    [newRow.RNmin, newRow.RNmax, newRow.RNmean] = get_current(pow_tbl, 'RN');
    newRow.RNmeanDay = (2500/newRow.RNmean)/24;
    
    tblPower = [tblPower; newRow];
end

writetable(tblPower, 'currentCons.csv')