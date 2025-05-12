# import core

# # Example class to represent a sensor module
# class Sensor:
#     def __init__(self, name):
#         self.name = name

# # Global instance of a Sensor
# my_sensor = Sensor("FrontDistanceSensor")

# # Callback when an obstacle is detected
# def on_obstacle_detected(sender, args):
#     # sender will be the Sensor object
#     if sender:
#         core.log(f"Signal received from: {sender}")

#     # args will be a dictionary with more information
#     if not args:
#         core.log("No args provided with signal.")
#         return
#     distance = args.get("distance", None)
#     if distance == None:
#         core.log("No distance provided in args.")
#         return
#     core.log(f"Obstacle detected at {distance} cm!")
#     if distance < 10:
#         core.log("Distance too close! Emitting 'move_backward' signal.")
#         core.signal_emit("move_backward", None, None)

# def init():
#     core.log("Running init()")

#     # Connect callback
#     core.signal_connect("obstacle_detected", on_obstacle_detected)

#     # Immediately emit obstacle_detected signal for testing
#     core.log("Emitting 'obstacle_detected' signal with sender and args...")
#     core.signal_emit(
#         "obstacle_detected",
#         my_sensor,                 # sender = our sensor object
#         {"distance": 5}             # args = a dictionary with distance
#     )

# def update(dt):
#     pass

# def shutdown():
#     core.log("Running shutdown()")
