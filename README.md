# Signal Tower

Signal towers (or stack lights) are usually used in industrial manufacturing and display the
state of a machine to the operators. They are mounted on a pole, which makes it easy to spot a
malfunctioning machine from far away. Different colors can be used to display various conditions
(for example green = normal, red = malfunction, yellow = warning, blue = good, but low on raw
materials).

Nowadays software development is also partly regarded as an industrial process. For example
continuous integration and deployment can be fully automated. When a developer pushes new code into
the central repository, this code can be automatically build, tested and deployed onto production
servers. Testing is an essential part of this process, since you don't want bad code to creep into
production.

A fast feedback of these problems to the developers helps them to quickly react accordingly. So
today more and more development teams are installing some kind of easily perceptible
good/broken-indicators. This can either be a big monitor which displays some kind of a build
status dashboard, or a stylish industrial signal tower like the one in the picture above.

This a custom shield for an Arduino which can control three signal tower lights. It is accessable 
via WLAN and the firmware implements a small HTTP server, which allows you to control the signal 
tower through some simple HTTP-REST-APIs.

[Read more about this project on my website...](http://www.grappendorf.net/projects/signal-tower.html)
