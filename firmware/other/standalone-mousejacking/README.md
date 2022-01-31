# EvilCrowRF-V2 Mousejacking

Sketch for using Evil Crow RF V2 as Mousejacking Mass Pwner Standalone Tool.
Once flashed into Evil Crow RF V2, it will automatically search and attack all vulnerable Mice & Keyboards around.

BE CAREFUL!

This is based on uCMousejack[1] and WHID Elite[2].
[1] https://github.com/phikshun/uC_mousejack
[2]https://github.com/whid-injector/whid-31337

P.S. This standalone mode doesn't need to be plugged into a PC to work. Is enough to plug the Evil Crow RF V2 on a USB-battery-pack or use a LiPo and enjoy a MouseDriving session. ;]

By default the payload executes cmd and writes a test text, setup to run into the Attack.h.
If you wanna create your own payload... go in the directory Payload_Generator_Mousejacking_Attack and:
1.- Edit ducky.txt and type your payload
2.- Run: ./attack_generator.py ducky.txt

This will create a new attack.h, you will have to use this in the mousejacking sketch
