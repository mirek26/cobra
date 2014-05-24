$COBRA_HOME/cobra mm3x3.py > /dev/null;
echo "0 0 3 4 1 1" | $COBRA_HOME/cobra-backend -m s .cobra.in
