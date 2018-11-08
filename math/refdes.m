classdef refdes
    
% This class records report data from the MAXREFDES169 into a cell array
%
% Example:
%   rd = refdes('COM15');
%   c = rd.report( 100, 'tracked temp' );
%   delete(rd);

    properties ( Access = private )
        serial_file
    end
    methods ( Access = private )
        function send_cmd( o,  cmd )
            fprintf( o.serial_file, cmd , 'async' );
            fgetl( o.serial_file );
        end
    end
    methods
        function o = refdes(comport)
            o.serial_file = serial( comport );
            set( o.serial_file, 'BaudRate', 115200 );
            set( o.serial_file, 'Terminator', 'CR' );
            fopen( o.serial_file' );
        end
        function lines = report( o, count, flags )
            if( o.serial_file.BytesAvailable > 0 )
                fread(o.serial_file, o.serial_file.BytesAvailable);
            end
            send_cmd( o, strcat('report', flags ) );
            for i=1:count
                s = fgetl( o.serial_file );
                lines{i} = s(find(~isspace(s)));
            end
            send_cmd( o, '\r' );
            pause(1);
            if( o.serial_file.BytesAvailable > 0 )
                fread(o.serial_file, o.serial_file.BytesAvailable);
            end
        end
        function delete( o )
            fclose( o.serial_file );
            delete( o.serial_file );
        end
    end
end