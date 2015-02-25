for FILE in ../zip/*.zip 
do 
unzip $FILE; mv *_dsc300.tif ../dsc/; rm -f *; 
done
