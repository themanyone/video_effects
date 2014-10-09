Team Tracker

 * The track element tracks and optionally marks areas of color in a 
 * video stream. A threshold value may be adjusted to eliminate false 
 * signals. The algorithm gives more weight to color, especialy skin 
 * tones, and pays less attention to shading and bad lighting.
 * During each frame, if the #GstTrack:message property is #TRUE,
 * track emits an element message for each video frame.