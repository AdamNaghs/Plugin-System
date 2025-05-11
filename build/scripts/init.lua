local counter = 0

-- Called when our own test signal is emitted
local function on_test_signal(sender, args)
    core.log("on_test_signal called!")

    -- If args exist and have data
    if args then
        core.log(string.format("Received args!"))
    end
end

function init()
    core.log("Lua script initialized (self-contained signal test).")

    -- Connect our handler to a test signal
    core.signal_connect("lua_test_signal", on_test_signal)

    -- Emit the test signal immediately to verify connection
    core.log("Emitting lua_test_signal now (init)...")
    core.signal_emit("lua_test_signal", nil, { hello = "world" })
end

function update(dt)
    -- Re-emit the signal every 3 seconds
    counter = counter + dt
    if counter >= 3.0 then
        core.log("3 seconds passed. Emitting lua_test_signal again...")
        core.signal_emit("lua_test_signal", nil, { time = counter })
        counter = 0
    end
end

function shutdown()
    core.log("Lua script shutting down cleanly.")
end
