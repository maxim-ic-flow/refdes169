% calculate ratio2thresh table

lsb = 1/128;    % value of the least significant bit of the MAX3510x t1/t2 ratio register

r = 0:127;

% constrain range based on test experience

r = r * lsb;

[a l] = min(abs(r-.1));
[a h] = min(abs(r-.9));

r = r(l:h);
s =1./cos(r*pi/2);    % inverse relation of t1/t2 ratio to threshold

fprintf('float_t:\r');
for i = 1:length(s)
    fprintf('%d, ',s(i));
end

fprintf('\rq16:\r');

for i = 1:length(s)
    fprintf('%d, ', round(s(i) * 2^14) );
end

fprintf('\r');