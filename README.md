# MPlayerInterlaceDetection
A small program I wrote do decide whether content is interlaced or not using mplayer(2). 

Since mplayer(2) itself does not detect if a source is interlaced or not I started a small project to analyse a source with mplayers pullup filter.

basically it collects the output of :
mplayer2 -noframedrop -lavdopts threads=8 -speed 100 -v "path to input" -nosound -vo null -ao null -nosub -vf pullup
and:
mplayer2 -noframedrop -lavdopts threads=8 -speed 100 -v "path  to input" -nosound -vo null -ao null -nosub -vf pp=lb,pullup

which both output stuff like:

V:  17.9 537/537 156% 513%  0.0% 0 0 100.00x
affinity: .0++1..2+.3.
breaks:   .0..1..2|.3.
duration: 2

per frame.

Depending if there's a + or a ++ between the numbers in affinity it counts light (+) and strong (++) affinities, whether or not there is a break and how the duration changed.

Looking at the affinity, breaks and duration changed it than decides if the content is:
a. normal interlaced
b. telecine
c. field-blended
d. mixed progressive/interlaced
content.

I know that this can't replace detection with your own eyes using (avisynth and separate fields&co), but it might be a nice gimmick for those that don't know how to use avisynth.

Since I normally only have telecine or interlaced content I play with (+ I'm a n00b when it comes to interlacing&co) this decision isn't really tested well.
