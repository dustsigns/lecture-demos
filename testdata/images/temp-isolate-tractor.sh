convert tractor.png -colorspace CMYK -separate y.png
convert y-1.png -threshold 65% -morphology close disk:10 -morphology dilate disk:25 tiremask.png
convert y-1.png -region 300x100 -contrast +region -fuzz 16% -fill black -draw "color 200,50 floodfill" -negate -threshold 99% -region 250x100+36+74 -morphology close plus +region -region 20x70+90+0 -morphology close octagon:2 +region -fill black -draw "rectangle 100,168 190,200" restmask.png
convert restmask.png tiremask.png -compose Lighten -composite mask.png
convert tractor.png mask.png -compose copy-opacity -composite tractor-s.png
