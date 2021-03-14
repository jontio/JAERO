clear all;
close all;
actual_input_include();
actual_output_include();
expected_input_include();
expected_output_include();
assert(numel(actual_input)==numel(expected_input));
assert(numel(actual_output)==numel(expected_output));
plot(real(actual_output));hold on;
plot(real(expected_output));hold off;
xlim([2048 numel(actual_output)])
legend({'actual output','expected output'});
title('real');
figure;
plot(imag(actual_output));hold on;
plot(imag(expected_output));hold off;
xlim([2048 numel(actual_output)])
legend({'actual output','expected output'});
title('imag');