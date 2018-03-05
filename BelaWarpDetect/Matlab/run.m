%% load audio
[y, fs] = audioread('audio.wav');
[template, template_fs] = audioread('template.wav');
if fs ~= template_fs
    error('Mismatched sampling rates.');
end

%% pass into dtm
[scores, len] = dtm(template, y, fs);

figure;
ax1 = subplot(2, 1, 1); plot(scores);
ax2 = subplot(2, 1, 2); plot(len);
linkaxes([ax2 ax1], 'x');

%% pass into match syllables
[scores, len] = match_syllables(y, fs, template);

figure;
ax1 = subplot(2, 1, 1); plot(scores);
ax2 = subplot(2, 1, 2); plot(len);
linkaxes([ax2 ax1], 'x');

%% 

scores(scores >= 2900) = nan;
len(abs(len) > 32) = nan;

figure;
ax1 = subplot(2, 1, 1); plot(scores);
ax2 = subplot(2, 1, 2); plot(len);
linkaxes([ax2 ax1], 'x');

%% setup
%pth = '~/Documents/School/BU/Gardner Lab/Syllable Match/';
pth = '/Volumes/Lab/Gardner Lab/Bela/lbr3009files/';
fl = 'lbr3009_annotation.mat';

load(fullfile(pth, fl));

%% load audio to make template
syllables = 1:25;
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

        % unique segments
        for j = idx'
            strt = round(elements{i}.segFileStartTimes(j) * fs);
            stop = round(elements{i}.segFileEndTimes(j) * fs);
            if (stop - strt) < 512
                continue;
            end
            audio{end + 1} = y(strt:stop);
        end
    end
    
    [tmpl, weights] = build_template(audio, fs);
    templates{syllable} = tmpl;
end

%% first pass: load syllables

syllables = {};

total = 0;

for i = 1:length(keys)
    if ~exist(fullfile(pth, keys{i}), 'file')
        continue;
    end
    
    % load audio
    [y, fs] = audioread(fullfile(pth, keys{i}));
    
    % unique segments
    for j = 1:length(elements{i}.segType)
        s = elements{i}.segType(j);
        if s < 1
            continue;
        end
        if s > length(syllables) || isempty(syllables{s})
            strt = round(elements{i}.segFileStartTimes(j) * fs);
            stop = round(elements{i}.segFileEndTimes(j) * fs);
            syllables{s} = y(strt:stop);
        end
    end
    
    % total
    total = total + length(elements{i}.segType);
end

%% second pass: score syllables

scores = nan(total, length(syllables));
lengths = nan(total, length(syllables));
actual = zeros(total, 1);

idx = 0;
for i = 1:length(keys)
    if ~exist(fullfile(pth, keys{i}), 'file')
        continue;
    end
    
    % nothing of interest?
    if isempty(elements{i}.segType)
        continue;
    end
    
    % load audio
    [y, fs] = audioread(fullfile(pth, keys{i}));
    
    % to evaluate
    to_evaluate = cell(1, length(elements{i}.segType));
    
    % unique segments
    for j = 1:length(elements{i}.segType)
        strt = round(elements{i}.segFileStartTimes(j) * fs);
        stop = round(elements{i}.segFileEndTimes(j) * fs);
        to_evaluate{j} = y(strt:stop);
        actual(idx + j) = elements{i}.segType(j);
    end
    
    % evaluate each
    for j = 1:length(syllables)
        if isempty(syllables{j})
            continue;
        end
        
        [cur_scores, cur_lengths] = eval_syllable(syllables{j}, fs, to_evaluate{:});
        
        scores((idx + 1):(idx + length(to_evaluate)), j) = cur_scores;
        lengths((idx + 1):(idx + length(to_evaluate)), j) = cur_lengths;
    end
    
    % update idx
    idx = idx + length(to_evaluate);
end

%% confusion matrix
[~, predicted] = max(scores, [], 2);
confusion = accumarray([actual(actual > 0) predicted(actual > 0)], 1, [length(syllables) length(syllables)]);
fprintf('Accuracy: %.2f%%\n', 100 * sum(diag(confusion)) ./ sum(confusion(:)));

%% plot roc
for i = 1:length(syllables)
    if isempty(syllables{i})
        continue;
    end
    
    % calculate correct
    correct = actual == i;
    
    % plot ROC
    figure;
    plotroc(1 * correct', scores(:, i)');
    title(sprintf('Syllable %d; N = %d', i, sum(correct)));
end

%% visualize
for i = [2 7]
    if isempty(syllables{i})
        continue;
    end
    
    if numel(syllables{i}) == length(syllables{i})
        [s, f, t] = spectrogram(syllables{i}, 512, 512-40, 512, fs);
        f_idx = f > 500 & f < 10000;
    else
        s = syllables{i};
        f = size(s, 1):-1:1;
        f_idx = f > 0;
        t = 1:size(s, 2);
    end

    figure;
    imagesc(t, f(f_idx), log(1 + abs(s(f_idx, :))));
    axis xy;
    title(sprintf('Syllable %d', i));
end
