let $bootstrap_sql_file = $MYSQLTEST_VARDIR/tmp/bootstrap.sql;
write_file $bootstrap_sql_file;
CREATE DATABASE test;
EOF
if ($USE_CONSENSUS_CLUSTER)
{
  let $MYSQLD_ARGS = --pid-file=$MYSQLD_DATADIR/mysqld-in-test.pid --secure_file_priv=$MYSQLTEST_VARDIR --socket=$MYSQLD_DATADIR/test.sock --skip-networking --skip-ssl --loose-skip-mysqlx --console --log-error-verbosity=3 --datadir=$MYSQLD_DATADIR $MYSQLD_EXTRA_ARGS > $MYSQLD_ERROR_LOG 2>&1 ;
  --exec $MYSQLD --no-defaults $MYSQLD_ARGS --initialize-insecure --init-file=$bootstrap_sql_file --raft-replication-cluster-info=$CLUSTER_INFO --raft-replication-cluster-id=1 --raft-replication-start-index=1
}

if (!$USE_CONSENSUS_CLUSTER)
{
  let $MYSQLD_ARGS = --pid-file=$MYSQLD_DATADIR/mysqld-in-test.pid --secure_file_priv=$MYSQLTEST_VARDIR --socket=$MYSQLD_DATADIR/test.sock --skip-log-bin --skip-networking --skip-ssl --loose-skip-mysqlx --console --log-error-verbosity=3 --datadir=$MYSQLD_DATADIR $MYSQLD_EXTRA_ARGS > $MYSQLD_ERROR_LOG 2>&1 ;
  --exec $MYSQLD --no-defaults $MYSQLD_ARGS --initialize-insecure --init-file=$bootstrap_sql_file
}

--remove_file $bootstrap_sql_file
