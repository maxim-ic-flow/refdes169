function o = parse_report(input)

% reads MAXREFDES169 report data from a file or from a cell array.
% cell arrays can be created using refdes.m
% files can be created by capturing USB serial data using terminal
% emulation software like PuTTY

    function [v, p] = parse_hex8( line, pos )
        % parses a 32-bit unsigned hex number
        v = double(typecast(uint8(sscanf( line(pos:pos+1), '%x')), 'int8'));
        p = pos + 2;
    end
    function [v, p] = parse_hex32( line, pos )
        % parses a 32-bit unsigned hex number
        v = double(typecast(uint32(sscanf( line(pos:pos+7), '%x')), 'int32'));
        p = pos + 8;
    end
    function [therm, reference ] = parse_temperature( line )
        % example: 't01B1CFFC0186DD99'
        % format:   xAAAAAAAABBBBBBBB
        % A = raw therm/cap discharge time
        % B = raw reference resistor/cap discharge time
        [therm, pos] = parse_hex32( line, 1 );
        reference = parse_hex32( line, pos );
    end
    function [up, down, up_period, down_period ] = parse_tracked( line )
        % example: 'x064AC31F064B2ACA0013EC8F00141C91'
        % format:   xAAAAAAAABBBBBBBBCCCCCCCCDDDDDDDD
        % A = raw up time-of-flight
        % B = raw down time-of-flight
        % C = raw up period
        % D = raw down period
        % see flowbody_sample_t in flowbody.h
        [up, pos] = parse_hex32( line, 1 );
        [down, pos] = parse_hex32( line, pos );
        [up_period, pos] = parse_hex32( line, pos );
        down_period = parse_hex32( line, pos );
    end
    function [ up, down, up_ratio, down_ratio ] = parse_tof( line )
        % example: 'r04799E4404DDC0B10541BBE705A5B69F0609AE34066E06E33F0479BC8504DDCA810541C91105A5C6850609C6C2066E7A1E40'
        % format:   rAAAAAAAABBBBBBBBCCCCCCCCDDDDDDDDEEEEEEEEFFFFFFFFGGGGGGGGHHHHHHHHIIIIIIIIJJJJJJJJKKKKKKKKLLLLLLLLMMNN
        
        % A through F = up hit register values 0-5
        % G through L = down hit register values 0-5
        % M = up t1/t2
        % N = down t1/t2
        
        up = zeros(1,6);
        down = zeros(1,6);
        pos = 1;
        for i=1:6
            [up(i), pos] = parse_hex32( line, pos );
        end
        for i=1:6
            [down(i), pos] = parse_hex32( line, pos );
        end
        [up_ratio, pos] = parse_hex8( line, pos );
        down_ratio = parse_hex8( line, pos );

    end

% using putty to capture serial tof & temp output from the refdes to
% create a csv file 'filename'

% output is a struct

if iscell( input )
    f = input;
else
    f = fopen(input);
end

line = 1;
tracked_ndx = 1;
tof_ndx = 1;
temperature_ndx = 1;
ndx = 1;

while 1
    if iscell( input )    
        if line > length(f)
            break;
        end
        s = f{line};
    else
        if feof(f)
            break;
        end
        s = fgetl(f);
    end    
    if( length(s)<1 || (~(strcmp(s(1),'x') || strcmp(s(1),'t') || strcmp(s(1),'r') ) ) )
        fprintf('Line %d ignored.\n',line);
        line = line + 1;
        continue
    end
    try
        if s(1) == 'x'
            [ o.tracked.up(tracked_ndx), o.tracked.down(tracked_ndx), o.tracked.up_period(tracked_ndx), o.tracked.down_period(tracked_ndx) ] = parse_tracked( s(2:end) );
            o.tracked.ndx(tracked_ndx) = ndx;
            tracked_ndx = tracked_ndx + 1;
            ndx = ndx + 1;
        elseif s(1) == 'r'
            [ o.tof.up(tof_ndx,:), o.tof.down(tof_ndx,:), o.tof.up_ratio(tof_ndx), o.tof.down_ratio(tof_ndx) ] = parse_tof( s(2:end) );
            o.tof.ndx(tof_ndx) = ndx;
            tof_ndx = tof_ndx +1;
            ndx = ndx + 1;
        elseif s(1) == 't'
            [ o.temperature.therm(temperature_ndx), o.temperature.reference(temperature_ndx) ] = parse_temperature( s(2:end) );
            o.temperature.ndx(temperature_ndx) = ndx;
            temperature_ndx = temperature_ndx +1;
        end
    catch ME
        fprintf('Line %d invalid.\n',line);
    end
    line = line + 1;
end
if ~iscell( input )    
    fclose(f);
end
end
