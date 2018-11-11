classdef refdes
    
% This class records report data from the MAXREFDES169 into a cell array
%
% Example:
%   rd = refdes('COM15');
%   c = rd.report( 100, 'tracked temp' );
%   delete(rd);

    properties ( Access = private )
        serial
    end
    methods ( Access = private )
        function tf = send_cmd( o,  cmd )
            flush(o);
            fprintf( o.serial, cmd , 'async' ); % mbed serial port requires 'async' for some reason
            e = fgetl( o.serial );
            if( length(e) == 0 )
                tf = false;
            else
                tf = true;
            end
        end
        function flush(o)
            if( o.serial.BytesAvailable > 0 )
                fread(o.serial, o.serial.BytesAvailable);
            end
        end
    end
    methods
        function o = refdes(comport)
            o.serial = serial( comport );
            set( o.serial, 'BaudRate', 115200 );
            set( o.serial, 'Terminator', 'CR' );
            set( o.serial, 'Timeout', 0.5 );
            fopen( o.serial' );
        end
        function tf = tofsr( o, sample_rate_hz )
            flush(o);
            tf = send_cmd( o, sprintf( 'tofsr=%d', sample_rate_hz ) );
        end
        function lines = report( o, count, flags )
            if( send_cmd( o, strcat('report ', flags ) ) == false )
                lines = [];
                return
            end
            c = 1;
            while c <= count
                s = fgetl( o.serial );
                s = s(find(~isspace(s)));
                if( isempty(s) )
                    lines= [];
                    return
                end
                lines{c} = s;
                c = c + 1;
            end
            send_cmd( o, '\r' );
            pause(0.5);
            flush(o);
        end
        function delete( o )
            fclose( o.serial );
            delete( o.serial );
        end
    end
end