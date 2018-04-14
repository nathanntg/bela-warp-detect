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

% 2D template
[scores, len] = match_syllables(y, fs, build_template({template}, fs));

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
