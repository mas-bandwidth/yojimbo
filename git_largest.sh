
#!/bin/bash
#set -x 

# Shows you the largest objects in your repo's pack file.
# Written for osx.
#
# @see http://stubbisms.wordpress.com/2009/07/10/git-script-to-show-largest-pack-objects-and-trim-your-waist-line/
# @author Antony Stubbs

# set the internal field spereator to line break, so that we can iterate easily over the verify-pack output
IFS=$'\n';

# list all objects including their size, sort by size, take top 10
objects=`git verify-pack -v .git/objects/pack/pack-*.idx | grep -Ev "non delta|chain length|git/objects" | tr -s " " | sort -k3gr | head -n30`

echo "All sizes are in kB's. The pack column is the size of the object, compressed, inside the pack file."

output="size,SHA,location"
for y in $objects
do
  # extract the size in KB
  size=$((`echo $y | cut -f 3 -d ' '`/1024))
  # extract the SHA
  sha=`echo $y | cut -f 1 -d ' '`
  # find the objects location in the repository tree
  other=`git rev-list --all --objects | grep $sha`
  #lineBreak=`echo -e "\n"`
  output="${output}\n${size},${other}"
done

echo -e $output | column -t -s ', '