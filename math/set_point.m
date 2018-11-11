function set_point( flow )
    mf = mass_flow('COM15');
    mf.set_point( flow );
    delete(mf);
end
