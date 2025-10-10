#/bin/bash

sed  $'s/ for(/INSTRUMENT;for(/' $1 | sed  's/ for (/INSTRUMENT;for(/' | sed  $'s/\tfor (/INSTRUMENT;for(/' | sed  $'s/\tfor(/INSTRUMENT;for(/' | sed  $'s/ while(/INSTRUMENT;while(/' | sed  's/ while (/INSTRUMENT;while(/' | sed  $'s/\twhile (/INSTRUMENT;while(/' | sed  $'s/\twhile(/INSTRUMENT;while(/' | sed  $'s/ if(/INSTRUMENT;if(/' | sed  's/ if (/INSTRUMENT;if(/' | sed  $'s/\tif (/INSTRUMENT;if(/' | sed  $'s/\tif(/INSTRUMENT;if(/' | sed '/malloc/ s/./INSTRUMENT;&/'  | sed '/+=/ s/./INSTRUMENT;&/' | sed '/printf/ s/./INSTRUMENT;&/'
