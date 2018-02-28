function compile_mex(varargin)
%COMPILE_MEX Compile MEX interface into the warp detector
%   No parameters are required, but you can optionally provide parameters:
%
%   COMPILE_MEX('debug', true) turns off optimizations, adds debugging
%   symbols to the binaries and turns on verbose compilation, all to help
%   debug issues with the MEX files.
%
%   COMPILE_MEX('warnings', true) turns on all warnings during compile
%   time (only tested with Clang compiler).

%% parameters
debug = false;
warnings = false;

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

% check platform
if ~ismac
    error('Only supports the macOS platform (requires Accelerate framework).');
end

% print nice message
fprintf('Compiling functions...\n');

c = {};
cf = {};
lf = {};

% add general include path
cf{end + 1} = '-I/usr/local/include';
lf{end + 1} = '-L/usr/local/lib';

% show warnings
if warnings
    cf{end + 1} = '-Weverything -Wno-c++98-compat';
end

% enable debugging
if debug
    c{end + 1} = '-g';
    c{end + 1} = '-v';
else
    c{end + 1} = '-silent';
    cf{end + 1} = '-O3';
end

% add parent directory as include path
root = fileparts(pwd);
cf{end + 1} = ['-I' root];

% build cflags
if numel(cf) > 0
    c{end + 1} = ['CFLAGS="\$CFLAGS ' strjoin(cf) '"'];
end

% include sndfile
lf{end + 1} = '-lsndfile.1';

% include Accelerate framework
if ismac
    lf{end + 1} = '-framework Accelerate';
end

c{end + 1} = ['LDFLAGS="\$LDFLAGS ' strjoin(lf) '"'];

% call mex functions
functions = {{'Matlab/dtm.cpp', 'Library/CircularShortTimeFourierTransform.cpp', 'Library/DynamicTimeMatcher.cpp'}, ...
    {'Matlab/match_syllables.cpp', 'Library/CircularShortTimeFourierTransform.cpp', 'Library/DynamicTimeMatcher.cpp', 'Library/LoadAudio.cpp', 'Library/MatchSyllables.cpp'}, ...
    {'Matlab/eval_syllable.cpp', 'Library/CircularShortTimeFourierTransform.cpp', 'Library/DynamicTimeMatcher.cpp', 'Library/LoadAudio.cpp', 'Library/MatchSyllables.cpp'}};
for j = 1:length(functions)
    if iscell(functions{j})
        fprintf('%s\n', functions{j}{1});
        d = [c cellfun(@(x) fullfile(root, x), functions{j}, 'UniformOutput', false)];
    else
        fprintf('%s\n', functions{j});
        d = [c fullfile(root, [functions{j} '.cpp'], 'UniformOutput', false)];
    end
    mex(d{:});
end
