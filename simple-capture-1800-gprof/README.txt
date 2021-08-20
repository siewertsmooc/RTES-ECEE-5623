For Gprof
---------
1) Build the test code with "-pg" flag in the Makfile
2) Run the 1800 frame test
3) Upon completion, you should have a "gmon.out" file
4) Run analysis with:

pi@raspberrypi:~/Downloads/simple-capture-1800-gprof $ ls
capture  capture.c  capture.o  frames  gmon.out  gprof-example-analysis.txt  Makefile
pi@raspberrypi:~/Downloads/simple-capture-1800-gprof $ gprof capture gmon.out > analysis.txt

Review analysis.txt

I have saved off an example from a run on my Raspberry Pi as gprof-example-analysis.txt

For Syslog
----------

1) Run this code
2) In another terminal window, type in "tail -f /var/log/syslog"
3) You should see events from syslog calls I have inserted into this demo code:

Aug  3 14:44:12 raspberrypi kernel: [1134113.944058] restoring control 00000000-0000-0000-0000-000000000101/3/7
Aug  3 14:44:14 raspberrypi capture: frame 2 read at 0.100612, @ 29.817436 FPS
Aug  3 14:44:14 raspberrypi capture: frame 3 read at 0.134899, @ 29.651856 FPS
Aug  3 14:44:14 raspberrypi capture: frame 4 read at 0.169191, @ 29.552350 FPS
Aug  3 14:44:14 raspberrypi capture: frame 5 read at 0.203423, @ 29.495194 FPS
Aug  3 14:44:14 raspberrypi capture: frame 6 read at 0.237670, @ 29.452644 FPS
Aug  3 14:44:14 raspberrypi capture: frame 7 read at 0.271896, @ 29.422988 FPS
Aug  3 14:44:14 raspberrypi capture: frame 8 read at 0.306102, @ 29.401954 FPS
Aug  3 14:44:14 raspberrypi capture: frame 9 read at 0.340325, @ 29.383709 FPS
Aug  3 14:44:14 raspberrypi capture: frame 10 read at 0.374247, @ 29.392390 FPS
Aug  3 14:44:15 raspberrypi capture: frame 11 read at 0.407996, @ 29.412084 FPS
Aug  3 14:44:15 raspberrypi capture: frame 12 read at 0.441772, @ 29.426965 FPS
Aug  3 14:44:15 raspberrypi capture: frame 13 read at 0.475549, @ 29.439638 FPS
Aug  3 14:44:15 raspberrypi capture: frame 14 read at 0.509307, @ 29.451781 FPS
Aug  3 14:44:15 raspberrypi capture: frame 15 read at 0.543077, @ 29.461728 FPS
Aug  3 14:44:15 raspberrypi capture: frame 16 read at 0.576892, @ 29.468251 FPS
Aug  3 14:44:15 raspberrypi capture: frame 17 read at 0.611328, @ 29.444074 FPS
Aug  3 14:44:15 raspberrypi capture: frame 18 read at 0.645897, @ 29.416445 FPS
Aug  3 14:44:15 raspberrypi capture: frame 19 read at 0.680441, @ 29.392685 FPS
Aug  3 14:44:15 raspberrypi capture: frame 20 read at 0.715108, @ 29.366190 FPS
...
