# megacutegrab
![](https://files.catbox.moe/jxlmsx.png)
Connect to your Hydrus database and objectively rank your images using advanced bayesian ranking!
Shows you two images at a time, and you pick which one is better.
It calculates the scores and inserts them into a Hydrus inn/decc scoring service so you can sort your images in hydrus

# Ranking Details: 
Using [Microsoft Trueskill](http://www.moserware.com/2010/03/computing-your-skill.html) we assign a score and a score confidence to tags and images, unlike chess elo Trueskill can rank team games, allowing us to assign scores to booru-tags and other metadata as well as just the image MD5 itself.

![](http://www.moserware.com/assets/computing-your-skill/TrueSkillCurvesBeforeExample.png)


# Installation
Developed on nix so the build process for me looks like this:
```
git clone --recursive https://github.com/keptan/megacute
nix develop
cmake .
make 
./megacute
```


