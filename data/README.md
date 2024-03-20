# File format

1. betree_defs
   * Format: `<variable name>|<type>|<allow undefined>|<min value>|<max value>`
   * Min value and max value are optional

2. betree_constants
    * Format: `expression id,campaign_group_id,campaign_id,advertiser_id,flight_id`

3. betree_exprs
    * List of boolean expressions. The count of betree_exprs and betree_constants should be the same.
    * Format: `<boolean expression>`

4. betree_events
   * List of query values in json format, where the key is the variable definition and value to match against the boolean expression
