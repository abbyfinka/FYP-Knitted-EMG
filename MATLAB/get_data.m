%% load EMG data

clear; clc; 
file_name = "hpf-standardelectrodes-x6gain-3";
get_path = "C:\Imperial\FYP-Knitted-EMG\App\Logs\";
lines = readlines(get_path + file_name + ".txt");
lines = split(lines(1:end-1), ':');


%% processing data into variables

laptop_timestamp = lines(:,1);
esp_timestamp = str2double(lines(:,2));
channel_data_as_string = lines(:,3);

data_count = length(channel_data_as_string);
num_channels = 8;
channel_data = zeros(data_count, num_channels);

for i = (1:data_count)
    channel_data(i,:) = str2double(split(extractBetween(channel_data_as_string(i),3,strlength(channel_data_as_string(i))-1), ','));
end

%% get sampling rate

sampling_rate = 1000;

%% store as .mat

put_path = "C:\Imperial\FYP-Knitted-EMG\MATLAB\Data\" + file_name + ".mat";
save(put_path, "laptop_timestamp", "esp_timestamp", "channel_data", "sampling_rate", "num_channels");

