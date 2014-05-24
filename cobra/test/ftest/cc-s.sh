$COBRA_HOME/cobra cc.py > /dev/null;
echo "4 2 3 0 1 0" | $COBRA_HOME/cobra-backend -m s .cobra.in
echo "0 2 2 2 2 2 2 2 2 2 2 0 2 0" | $COBRA_HOME/cobra-backend -m s .cobra.in
