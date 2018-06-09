function [new_audio] = review_audio(audio, fs, varargin)
%REVIEW_AUDIO Summary of this function goes here
%   Detailed explanation goes here

%% parameters
max_instances = [];

% load custom parameters
nparams = length(varargin);
if 0 < mod(nparams, 2)
    error('Parameters must be specified as parameter/value pairs');
end
for i = 1:2:nparams
    nm = lower(varargin{i});
    if ~exist(nm, 'var')
        error('Invalid parameter: %s.', nm);
    end
    eval([nm ' = varargin{i+1};']);
end

%% run
new_audio = {};

window_length = 1024; % samples
window_stride = 128; % samples
freq_range = [500 12000];

for i = 1:length(audio)
    % window
    window = hanning(window_length);
    nfft = 2^ceil(log2(window_length));

    % calculate spectrogram
    [s, f, t] = spectrogram(audio{i}, window, window_length - window_stride, nfft, fs);
    
    % frequencies of interest
    f_idx = f >= freq_range(1) & f <= freq_range(2);
    
    % display figure
    h = figure;
    imagesc(t, f(f_idx), 1 + log(abs(s(f_idx, :))));
    title(sprintf('Audio %d of %d', i, length(audio)));
    xlabel('Time [s]');
    ylabel('Freq [Hz]');
    axis xy;
    
    % wait for key press
    while 1 ~= waitforbuttonpress()
    end
    
    % check keypress
    if strcmp(h.CurrentCharacter, 'y')
        % append new audio
        new_audio{end + 1} = audio{1};
        
        % check max instances
        if ~isempty(max_instances) && length(new_audio) >= max_instances
            close(h);
            break;
        end
    end
    
    % close window
    close(h);
end

end

