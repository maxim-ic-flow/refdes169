function [s,flow] = profile_multiple( config, count )

    s=[0 0.4:0.2:3.0 3.5:0.5:5.0 6.0:2.0:42.0 ];
    flow=zeros(count,length(s));
    for i=1:count
        tracked = profile_flow( config, s, 100, 500, 'tracked temp' );
        tof = tracked_to_tof( tracked );
        flow(i,:) = raw_flow( tof.up, tof.down ); % compensate for transducer offset
    end
end