#
# Include this script to wait until the connection to the
# server has been restored or timeout occurs
#
# $wait_counter - can be set before calling the script, will be reset at the end
#
# $explicit_default_wait_counter - can be set before calling the script,
#                             reset only at the end of the test, overrides $wait_counter
#
# $add_wait_counter - can be set before calling the script,
#                     additional time count to wait during valgrind run.

--disable_result_log
--disable_query_log

--let $counter= 1000
if ($explicit_default_wait_counter)
{
  --let $wait_counter= $explicit_default_wait_counter
}
if ($VALGRIND_TEST) {
  --let $wait_counter= 20000
  if ($add_wait_counter) {
    --expr $wait_counter= $wait_counter + $add_wait_counter
  }
}
if ($TSAN_OPTIONS) {
  --let $wait_counter= 15000
}
if ($MTR_MANUAL_DEBUG) {
  --let $counter = 864000
}
if ($wait_counter) {
  --let $counter= $wait_counter
}
--let $mysql_errno= 9999
while ($mysql_errno)
{

  # Strangely enough, the server might return "Too many connections"
  # while being shutdown, thus 1040 is an "allowed" error
  # See BUG#36228
  --error 0,1040,1053,2002,2003,2006,2013,1045,ER_SECURE_TRANSPORT_REQUIRED,2016,2017,ER_CLIENT_INTERACTION_TIMEOUT
  show session status;
  if ($mysql_errno == 1045){
    --let mysql_errno=0
  }
  if ($mysql_errname == ER_SECURE_TRANSPORT_REQUIRED){
    --let mysql_errno=0
  }
  --dec $counter
  if (!$counter)
  {
    --die Server failed to restart
  }
  --sleep 0.1
}

if ($SECONDARY_ENGINE_TEST) {
  --let $secondary_engine_bootstrap = query_get_value($SECONDARY_ENGINE_BOOTSTRAP_QUERY, Value, 1)
  if ( $secondary_engine_bootstrap == ON ) {
    --let $counter = 100
    --let $secondary_engine_status = OFF

    while ($secondary_engine_status == OFF) {
      --let $secondary_engine_status = query_get_value($SECONDARY_ENGINE_STATUS_QUERY, Value, 1)
      --dec $counter

      if (!$counter) {
        --die Secondary engine server is not running or it is not bootstrapped
      }

      --sleep 0.1
    }

    --sleep 0.1

    # Wait till the tables get loaded into secondary engine
    --perl
    my $wait_file= "$ENV{MYSQLTEST_VARDIR}/tmp/wait_until_load";
    while (-e $wait_file) {
      select(undef, undef, undef, (100 / 1000));
    }
    EOF
  }
}

if ($USE_CONSENSUS_CLUSTER)
{
  --let $CONSENSUS_IS_ENABLE_IN_THIS_MYSQLD = `select count(*)!=0 from information_schema.wesql_cluster_local`
  if($CONSENSUS_IS_ENABLE_IN_THIS_MYSQLD)
  {
    # wait leader election
    if (!$wait_follower)
    {
      # wait leader from consensus
      --let $counter = 6000
      --let $check_counter = 10
      while ($counter > 0) {
        let $success= `select count(*)=1 from information_schema.wesql_cluster_local where role='leader'`;
        if ($success) {
          let $counter= 0;
        }
        if (!$success) {
          --dec $check_counter
          if ($check_counter == 0) {
            call dbms_consensus.force_promote();
            let $check_counter= 10;
          }
          --sleep 0.1
          --dec $counter
        }
      }

      # wait leader enable read/write
      let $wait_timeout= 60;
      let $wait_condition= select SERVER_READY_FOR_RW='Yes' from information_schema.wesql_cluster_local;
      --source include/wait_condition.inc
    }
  }
}

# Reset $wait_counter so that its value won't be used on subsequent
# calls, and default will be used instead.
if (!$explicit_default_wait_counter)
{
  --let $wait_counter= 0
}
--enable_query_log
--enable_result_log
