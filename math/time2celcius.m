function celcius = time2celcius( therm, reference )

% returns degrees celcius given the ratio of the time values
% provided by the MAX3510x

    function tempK = rtd(ratio)
        
        % converts a resistance ratio to a temperature value
        % assumes the thermo-resistive device is an RTD with
        % specific characteristics.  May need to change for
        % your device.
        
        r0 = 1000;
        a = 3.9083e-3;  % a and b are dependant on the RTD
        b = -5.775e-7;
    
        c1 = 4 * r0 * b;
        c2 = a*a*r0*r0;
        c3 = -a * r0;
        c4 = 2 * r0 * b;

        dr = r0 - ratio;

        rad = sqrt( c2 - c1 * dr );
        tempK = (( c3 + rad ) / c4);
    end
    celcius = rtd( 1000 * therm ./ reference );
end