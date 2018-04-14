function [tmpl, weights] = build_template(audio, fs, varargin)
%BUILD_TEMPLATE Combine audio as a syllable template
%   Builds spectrograms, warps columns to match and returns both a template
%   and weight function that can be used for matching.

    %% parameters
    
    figures = false;
    alpha = [];
    window_length = 512; % samples
    window_stride = 40; % samples
    freq_range = [1e3 1e4];
    log_power = true;

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

    %% get base template
    audio_len = cellfun(@length, audio);
    [~, idx] = sort(audio_len);
    tmpl_idx = idx(ceil(length(idx) / 2));

    %% make spectrograms
    [s, f, t] = to_spectrogram(audio{tmpl_idx}, fs);
    
    % 
    spect_len = zeros(1, length(audio));
    spect_len(tmpl_idx) = size(s, 2);
    
    % allocate space for spectrograms
    spects = zeros(size(s, 1), size(s, 2), length(audio), 'like', s);
    spects(:, :, tmpl_idx) = s;
    
    % set alpha
    if isempty(alpha)
        cols = size(s, 2);
        alpha = 2 + 1 * 0.9 .^ ([0:(ceil(cols / 2) - 1) (floor(cols / 2) - 1):-1:0]);
    end
    
    % for others
    for i = 1:length(audio)
        if i == tmpl_idx
            continue;
        end
        
        % calculate spectrogram
        s2 = to_spectrogram(audio{i}, fs);
        spect_len(i) = size(s2, 2);
        
        % warp
        path = dtw_path(s, s2, alpha);
        
        % store
        spects(:, path(1, :), i) = s2(:, path(2, :));
    end
    
    %% visualize
    spects_max = max(max(spects, [], 1), [], 2);
    spects_norm = bsxfun(@rdivide, spects, spects_max);
    
    if figures
        figure;
        imagesc(t, f, mean(spects, 3));
        colorbar;

        figure;
        imagesc(t, f, std(spects_norm, 0, 3));
        colorbar;
    end
    
    %% return template
    tmpl = mean(spects, 3);
    weights = 1 ./ (1 + std(spects_norm, 0, 3));
    
    %% print audio length statistics
    fprintf('Mean: %.1f columns\n', mean(spect_len));
    fprintf('Median: %.1d columns\n', median(spect_len));
    fprintf('Min: %.1f columns\n', min(spect_len));
    fprintf('5th percentile: %.1f columns\n', prctile(spect_len, 5));
    fprintf('25th percentile: %.1f columns\n', prctile(spect_len, 25));
    fprintf('75th percentile: %.1f columns\n', prctile(spect_len, 75));
    fprintf('95th percentile: %.1f columns\n', prctile(spect_len, 95));
    fprintf('Max: %.1f columns\n', max(spect_len));

    %% help functions

    function [s, f, t] = to_spectrogram(audio, fs)
        % zero pad to spectrogram boundary
        len = window_length + window_stride * ceil((length(audio) - window_length) / window_stride);
        audio = [audio; zeros(len - length(audio), 1)];
        
        % window
        window = hanning(window_length);
        nfft = 2^ceil(log2(window_length));
        
        % calculate spectrogram
        [s, f, t] = spectrogram(audio, window, window_length - window_stride, nfft, fs);
        
        % select frequency range
        s = s(f >= freq_range(1) & f < freq_range(2), :);
        
        % calculate power
        s = abs(s);
        if log_power
            s = log(1 + s);
        end
    end
end
