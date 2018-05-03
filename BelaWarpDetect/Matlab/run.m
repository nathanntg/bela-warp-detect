%% setup
pth = '/Volumes/screening/llb3/';
fl = 'llb3_annotation_ordered.mat';

load(fullfile(pth, fl));

% if you change these constants, you must also change the C++ defaults
window_length = 512;
window_stride = 60;
log_power = false;

%% optional: combine consecutive syllable 3's
oldElements = elements;
newSyllableType = 33;
for i = 1:length(elements)
    type = elements{i}.segType(:);
    fileStartTimes = elements{i}.segFileStartTimes(:);
    fileEndTimes = elements{i}.segFileEndTimes(:);
    
    idx = type(1:(end - 1)) == 3 & type(2:end) == 3 & fileEndTimes(1:(end-1)) + 0.05 > fileStartTimes(2:end);
    
    elements{i}.segAbsStartTimes = elements{i}.segAbsStartTimes([idx; false]);
    elements{i}.segFileStartTimes = elements{i}.segFileStartTimes([idx; false]);
    elements{i}.segFileEndTimes = elements{i}.segFileEndTimes([false; idx]);
    elements{i}.segType = ones(sum(idx), 1) * newSyllableType;
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
    end
    
    if syllable == 1 || (exist('newSyllableType', 'var') && newSyllableType == syllable)
        build_template(audio, fs, 'figures', true, 'window_length', window_length, 'window_stride', window_stride, 'log_power', log_power);
    end
end

%% second pass: score templates
%compile_mex;

scores = nan(0, length(templates));
lengths = nan(0, length(templates));
actual = zeros(0, 1);

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
    for j = 1:length(templates)
        if isempty(templates{j})
            continue;
        end
        
        [cur_scores, cur_lengths] = eval_syllable(templates{j}, fs, to_evaluate{:});
        
        scores((idx + 1):(idx + length(to_evaluate)), j) = cur_scores;
        lengths((idx + 1):(idx + length(to_evaluate)), j) = cur_lengths;
    end
    
    % update idx
    idx = idx + length(to_evaluate);
end

% confusion matrix
[~, predicted] = max(scores, [], 2);
confusion = accumarray([actual(actual > 0)' predicted(actual > 0)], 1, [length(templates) length(templates)]);
fprintf('Accuracy: %.2f%%\n', 100 * sum(diag(confusion)) ./ sum(confusion(:)));

known = ~cellfun(@isempty, templates);
fprintf('Accuracy: %.2f%%\n', 100 * sum(diag(confusion(known, known))) ./ sum(sum(confusion(known, known))));

%% confusion matrix
figure;
imagesc(bsxfun(@rdivide, confusion, sum(confusion, 2)));
axis square;
title('Confusion matrix');

figure;
imagesc(bsxfun(@rdivide, confusion(known, known), sum(confusion(known, known), 2)));
axis square;
title('Known confusion matrix');

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

%% softmax like scores
scores_norm = bsxfun(@rdivide, exp(scores), nansum(exp(scores), 2));
for i = 1:length(templates)
    if isempty(templates{i})
        continue;
    end
    
    % calculate correct
    correct = actual == i;
    
    % plot ROC
    figure;
    plotroc(1 * correct, scores_norm(:, i)');
    title(sprintf('Syllable %d; N = %d', i, sum(correct)));
end

%% visualize
for i = [2 7]
    if isempty(syllables{i})
        continue;
    end
    
    if numel(syllables{i}) == length(syllables{i})
        [s, f, t] = spectrogram(syllables{i}, window_length, window_length - window_stride, window_length, fs);
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

%% evaluate full audio
ne_templates = templates(cellfun(@(x) ~isempty(x), templates));
ne_templates_lu = zeros(1, length(templates));
ne_templates_lu(cellfun(@(x) ~isempty(x), templates)) = 1:length(ne_templates);

all_predict = cell(length(keys), length(templates));
all_scores = cell(length(keys), length(templates));
all_truth = cell(length(keys), length(templates));

len_threshold = 0.25;

idx = 0;
for i = 1:length(keys)
    if ~exist(fullfile(pth, keys{i}), 'file')
        continue;
    end
    
    % can be enabled or disabled
    % true: much faster, only bother with annotated audio
    % false: slower, get results for all audio files
    if true && isempty(elements{i}.segType)
        continue;
    end
    
    % load audio
    [y, fs] = audioread(fullfile(pth, keys{i}));
    
    % score
    [scores, len] = match_syllables(y, fs, ne_templates{:});
    tms = (window_length + window_stride * (0:(size(scores, 1) - 1))) ./ fs;
    
    % process scores
    scores_norm = bsxfun(@rdivide, scores .^ 2, sum(scores, 2) + eps);
    scores_mask = scores;
    for j = 1:length(ne_templates)
        idx = scores(:, j) > 0.2 & abs(len(:, j)) < len_threshold * size(ne_templates{j}, 2);
        scores_mask(~idx, j) = 0;
        
        % get peaks
        [pks, locs] = findpeaks(scores_mask(:, j), tms);
        
        t = find(ne_templates_lu == j);
        all_predict{i, t} = locs;
        all_scores{i, t} = pks;
        all_truth{i, t} = elements{i}.segFileEndTimes(j == ne_templates_lu(elements{i}.segType));
    end
    
    % find peaks
    if i == 1
        figure;

        ax1 = subplot(3, 1, 1);
        [s, f, t] = spectrogram(y, window_length, window_length - window_stride, window_length, fs);
        f_idx = f > 500 & f < 10000;
        imagesc(t, f(f_idx), log(1 + abs(s(f_idx, :))));
        clrs = bone(256);
        colormap(clrs(end:-1:1, :));
        axis xy;

        ax2 = subplot(3, 1, 2); 
        plot(tms, scores);
        r = ylim; r = [min(r(1), 0) max(r(2), 1)];
        clrs = lines(length(ne_templates));
        for j = 1:length(elements{i}.segType)
            tm = elements{i}.segFileEndTimes(j);
            if ne_templates_lu(elements{i}.segType(j)) > 0
                clr = clrs(ne_templates_lu(elements{i}.segType(j)), :);
            else
                clr = [0.5 0.5 0.5];
            end
            line([1 1] * tm, r, 'Color', clr);
        end
        % hold on;
        % plot(locs, pks, 'rx');
        % hold off;

        ax3 = subplot(3, 1, 3); 
        plot(tms, len);
        r = ylim;
        clrs = lines(length(ne_templates));
        for j = 1:length(elements{i}.segType)
            tm = elements{i}.segFileEndTimes(j);
            if ne_templates_lu(elements{i}.segType(j)) > 0
                clr = clrs(ne_templates_lu(elements{i}.segType(j)), :);
            else
                clr = [0.5 0.5 0.5];
            end
            line([1 1] * tm, r, 'Color', clr);
        end

        linkaxes([ax1 ax2 ax3], 'x');

        % limit zoom
        h = zoom;
        h.Motion = 'horizontal';
        h.Enable = 'on';
    end
end

%% evaluate results from above
for i = 1:length(templates)
    % has template?
    if isempty(templates{i})
        continue;
    end
    
    % get unique scores?
    scores = unique(sort(cat(1, all_scores{:, i})));
    
    best_threshold = [];
    best_accuracy = 0;
    best_confusion = [];
    
    for j = 1:length(scores)
        threshold = scores(j);
        confusion = zeros(2, 2);
        for k = 1:length(keys)
            if isempty(elements{k}.segType)
                continue;
            end
            predict = all_predict{k, i};
            c = score_predict(all_truth{k, i}, predict(all_scores{k, i} >= threshold));
            confusion = confusion + c;
        end
        accuracy = (confusion(1, 1) + confusion(2, 2)) / sum(confusion(:));
        if accuracy > best_accuracy
            best_threshold = threshold;
            best_accuracy = accuracy;
            best_confusion = confusion;
        end
    end
    
    fprintf('** Syllable %d **\n', i);
    fprintf('Accuracy: %.1f%%\n', best_accuracy * 100);
    fprintf('Threshold: %.6f\n', best_threshold);
    fprintf('Confusion:\n');
    disp(best_confusion);
end

%% show results
syllable = 1;
threshold = 0.467232;

for i = 1:length(keys)
    if ~exist(fullfile(pth, keys{i}), 'file')
        continue;
    end
    
    if isempty(elements{i}.segType)
        continue;
    end
    
    % load audio
    [y, fs] = audioread(fullfile(pth, keys{i}));
    
    % score
    [scores, len] = match_syllables(y, fs, templates{syllable});
    tms = (window_length + window_stride * (0:(size(scores, 1) - 1))) ./ fs;
    
    % process scores
    scores_mask = scores;
    scores_mask(scores < threshold | abs(len) > len_threshold * size(templates{syllable}, 2)) = 0;
    
    % find peaks
    if any(scores > 0)
        [pks, locs] = findpeaks(scores_mask, tms);
    else
        pks = [];
        locs = [];
    end
    
    figure;

    ax1 = subplot(3, 1, 1);
    [s, f, t] = spectrogram(y, window_length, window_length - window_stride, window_length, fs);
    f_idx = f > 500 & f < 10000;
    imagesc(t, f(f_idx), log(1 + abs(s(f_idx, :))));
    clrs = bone(256);
    colormap(clrs(end:-1:1, :));
    axis xy;

    ax2 = subplot(3, 1, 2); 
    plot(tms, scores);
    r = ylim; r = [min(r(1), 0) max(r(2), 1)];
    for j = 1:length(elements{i}.segType)
        tm = elements{i}.segFileEndTimes(j);
        if elements{i}.segType(j) == syllable
            clr = [0 0 1];
        else
            clr = [0.5 0.5 0.5];
        end
        line([1 1] * tm, r, 'Color', clr);
    end
    hold on;
    plot(locs, pks, 'rx');
    hold off;

    ax3 = subplot(3, 1, 3); 
    plot(tms, len);
    r = ylim;
    for j = 1:length(elements{i}.segType)
        tm = elements{i}.segFileEndTimes(j);
        if elements{i}.segType(j) == syllable
            clr = [0 0 1];
        else
            clr = [0.5 0.5 0.5];
        end
        line([1 1] * tm, r, 'Color', clr);
    end

    linkaxes([ax1 ax2 ax3], 'x');

    % limit zoom
    h = zoom;
    h.Motion = 'horizontal';
    h.Enable = 'on';
end