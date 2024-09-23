# auth_sec wrapper arround assert_binlog_event.inc
# Parameters:
# [--let invert= 0 | 1] Whether to match pattern or the absence of it
# [--let event= EVENT_PATTERN] Event pattern (see assert_binlog_event.inc)
# This file always sets $dont_print_pattern to 1

--let $gtid_event= Gtid
if (!$gtid_mode_on)
{
  --let $gtid_event= Anonymous_Gtid
}


if (!$invert)
{
  --echo # This event sequence pattern MUST be present in binlog: $event
}

if ($invert == 1)
{
  --echo # This event sequence pattern MUST NOT be present in binlog: $event
}

--let $dont_print_pattern= 1

if ($USE_CONSENSUS_CLUSTER)
{
  --let $event_sequence= Consensus_log # $gtid_event # $event
}

if (!$USE_CONSENSUS_CLUSTER)
{
  --let $event_sequence= $gtid_event # $event
}

--source include/assert_binlog_events.inc
