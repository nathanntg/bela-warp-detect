%% setup
pth = '~/Documents/School/BU/Gardner Lab/Syllable Match/llb3/summer';
fl = 'llb3_annotation_06102018.mat';

load(fullfile(pth, fl));

window_length = 256;
window_stride = 60;
log_power = false;

bela_fs = 44100;

%% make dir
if ~exist(fullfile(pth, 'bela'), 'dir')
    mkdir(pth, 'bela');
end

%% list of syllables
segType = cellfun(@(x) x.segType, elements, 'UniformOutput', false);
segType = sort(unique(cat(1, segType{:})));
segType = segType(:)';

%% first pass: load audio to make template
syllables = segType;
templates = cell(1, max(syllables));
for syllable = syllables
    audio = {};
    
    % for each file
    for i = 1:length(keys)
        if ~exist(fullfile(pth, keys{i}), 'file')
            continue;
        end

        % get all copies 
        idx = find(elements{i}.segType == syllable);
        if isempty(idx)
            continue;
        end

        % load audio
        [y, fs] = audioread(fullfile(pth, keys{i}));
        if size(y, 2) > 1
            y = y(:, 1);
        end
        
        % resample
        if bela_fs ~= fs
            y = resample(y, bela_fs, fs);
            fs = bela_fs;
        end

        % unique segments
        for j = idx'
            strt = floor(elements{i}.segFileStartTimes(j) * fs);
            stop = ceil(elements{i}.segFileEndTimes(j) * fs);
            if (stop - strt) < (window_length / 2)
                continue;
            end
            
            % add a little padding
            strt = max(strt - 256, 1);
            stop = min(stop + 256, length(y));
            
            audio{end + 1} = y(strt:stop);
        end
    end
    
    if length(audio) >= 5
        [tmpl, weights] = build_template(audio, fs, 'window_length', window_length, 'window_stride', window_stride, 'log_power', log_power);
        
        templates{syllable} = tmpl;
        
        if syllable >= 1000
            nm = sprintf('syllable%04d.bin', syllable);
        elseif syllable >= 100
            nm = sprintf('syllable%03d.bin', syllable);
        else
            nm = sprintf('syllable%02d.bin', syllable);
        end
        v = single(tmpl);
        fh = fopen(fullfile(pth, 'bela', nm), 'w');
        fwrite(fh, v(:), 'single');
        fclose(fh);
    end
end
