classdef mass_flow
    
    % This class impliments a control interface to a Parker Digital Mass Flow Regulator.
    % This mass flow controller was used to develop and test the
    % MAXREFDES169 and prototype flowbodies.
    
    properties ( Access = private )
        serial_file
    end
    methods
        function o = mass_flow(comport)
            o.serial_file = serial( comport );
            set( o.serial_file, 'BaudRate', 38400 );
            set( o.serial_file, 'Terminator', 'LF' );
            fopen( o.serial_file );
            fprintf( o.serial_file, sprintf( ':050301010412%c', 13 ) );
            fgetl( o.serial_file );
        end
        function set_point( o, lpm )
            
            % the valid flow range for thie mass flow regulator
            % is 0.4LPM to 42.0LPM
            % values less than 0.4LPM are interpreted as zero flow
            % values over 42.0LPM are clamped to 42.0LPM
            
            v = uint16(lpm / 42 * 32000);
            s = sprintf( ':0603010121%4.4X%c', v, 13 );
            fprintf( o.serial_file, s );
            fgetl( o.serial_file );
        end
        function delete( o )
            fclose( o.serial_file );
            delete( o.serial_file );
        end
    end
end