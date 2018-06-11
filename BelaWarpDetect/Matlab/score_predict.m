function confusion = score_predict(actual, predict)
%SCORE_PREDICT Summary of this function goes here

% no predictions? all actual entries are false negatives
if isempty(predict)
    confusion = [0, 0; length(actual), 0];
    return;
end

% debounce predict
if length(actual) > 1
    tm = min(diff(actual)) / 2;
    tm = max(min(tm, 0.1), 0.01); % bound to reasonable values
else
    tm = 0.05;
end

% no actual? nothing to detect
if isempty(actual)
    tp = 0;
    fn = 0;
else
    % match actual and predicted
    df = abs(bsxfun(@minus, actual(:), predict(:)'));

    a = min(df, [], 1);
    b = min(df, [], 2);

    tp = sum(b < tm);
    fn = length(actual) - tp;
end

% poor predictions (debounce poor predictions (only count once per tm,
% basically)
poor_predict = predict(a > tm);
if isempty(poor_predict)
    fp = 0;
else
    fp = 1 + sum(diff(poor_predict) > tm);
end

confusion = [0, fp; fn, tp];
    
end

