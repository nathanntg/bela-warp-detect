%% setup
pth = '~/Documents/School/BU/Gardner Lab/Syllable Match/llb3';
fl = 'llb3_annotation_ordered.mat';

load(fullfile(pth, fl));

%% explore annotations
for i = 1:length(keys)
    if ~exist(fullfile(pth, keys{i}), 'file')
        continue;
    end
    
    % no annotations?
    if isempty(elements{i}.segType)
        continue;
    end
    
    % load audio
    [y, fs] = audioread(fullfile(pth, keys{i}));
    
    % make spectrogram
    [s, f, t] = spectrogram(y, 512, 512-40, 512, fs);
    f_idx = f > 500 & f < 10000;
    imagesc(t, f(f_idx), imadjust(log(1 + abs(s(f_idx, :)))));
    clrs = bone(256);
    colormap(clrs(end:-1:1, :));
    axis xy;
    
    r = ylim;
    clrs = lines(30);
    for j = 1:length(elements{i}.segType)
        clr = clrs(elements{i}.segType(j), :);
        line([1 1] * elements{i}.segFileStartTimes(j), r, 'Color', clr);
        line([1 1] * elements{i}.segFileEndTimes(j), r, 'Color', clr);
    end
end
