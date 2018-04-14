function confusion = score_predict(actual, predict)
%SCORE_PREDICT Summary of this function goes here

if isempty(predict)
    confusion = [0, 0; length(actual), 0];
    return;
end

if isempty(actual)
    confusion = [0, length(predict); 0, 0];
    return;
end

% debounce predict
if length(actual) > 1
    tm = min(0.05, min(diff(actual)) / 2);
else
    tm = 0.05;
end
debounce = predict(1:(end-1)) + tm;
to_debounce = predict(2:end) < debounce;
to_debounce = [false; to_debounce(:)];
predict(to_debounce) = [];

% match actual and predicted
df = abs(bsxfun(@minus, actual(:), predict(:)'));

a = min(df, [], 1);
b = min(df, [], 2);

tp = sum(a < 0.05);
fp = length(predict) - tp;
fn = sum(b >= 0.05);

confusion = [0, fp; fn, tp];
    

end

