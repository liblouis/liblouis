WORKING_FILE=${1}
CONTRACTIONS_TABLE=${2}
BASE_TABLE=${3}
RULE_GREP="^[ \t]*[+-]\?\(nocross\|syllable\*\|word\|begword\|endword\)"
set -e
set -x
if [ -e $WORKING_FILE ]; then
	cat $WORKING_FILE | grep -v "^$" | grep -v "^#" > tmp || true
	mv tmp $WORKING_FILE
	if cat $WORKING_FILE | grep "$RULE_GREP" > tmp; then
		bash wrap_python.sh submit_rules.py -t $BASE_TABLE tmp $CONTRACTIONS_TABLE > tmp2
		mv tmp2 tmp
		cat <<- EOF > $CONTRACTIONS_TABLE
			##################################
			# don't edit this file directly! #
			##################################
		EOF
		cat tmp >> $CONTRACTIONS_TABLE
		cat $WORKING_FILE | grep -v "$RULE_GREP" > tmp || true
		mv tmp $WORKING_FILE
	fi
	if [ -s $WORKING_FILE ]; then
		rm $WORKING_FILE
	fi
fi
