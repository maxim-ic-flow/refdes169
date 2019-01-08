function setflow( config, flow )
    mf = mass_flow( config.massflow_comport );
    mf.set_point( flow );
    delete(mf);
end
