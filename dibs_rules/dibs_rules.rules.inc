<?php

function dibs_rules_rules_event_info() {

  $variables = array(
    'dibs_transaction' => array(
      'label' => t('Dibs transaction'),
      'type' => 'dibs_transaction',
      'description' => t('Description'),
      'skip save' => TRUE,
    ),
    /*
    'dibs_parameters' => array(
      'label' => t('Dibs parameters'),
      'type' => 'text',
      'description' => t('Description'),
    ),
    */
    /*
    'status' => array(
      'label' => t('Payment status'),
      'type' => 'text',
      'description' => t('Description'),
    ),
    'action_code' => array(
      'label' => t('Action code'),
      'type' => 'text',
      'description' => t('Description'),
    ),
    'acquirer' => array(
      'label' => t('Acquirer'),
      'type' => 'text',
      'description' => t('Description'),
    ),
    'capture_status' => array(
      'label' => t('Capture status'),
      'type' => 'text',
      'description' => t('Description'),
    ),
    */
  );

  return array(
    'dibs_rules_payment_accepted' => array(
      'label' => t('Payment accepted'),
      'group' => t('Dibs'),
      'access callback' => 'dibs_rules_rules_access',
      'variables' => $variables,
    ),
    'dibs_rules_payment_declined' => array(
      'label' => t('Payment declined'),
      'group' => t('Dibs'),
      'access callback' => 'dibs_rules_rules_access',
      'variables' => $variables,
    ),
    'dibs_rules_payment_pending' => array(
      'label' => t('Payment pending'),
      'group' => t('Dibs'),
      'access callback' => 'dibs_rules_rules_access',
      'variables' => $variables,
    ),
    'dibs_rules_payment_canceled' => array(
      'label' => t('Payment pending'),
      'group' => t('Dibs'),
      'access callback' => 'dibs_rules_rules_access',
      'variables' => $variables,
    ),
    'dibs_rules_invalid_order_information' => array(
      'label' => t('Recieved invalid order information'),
      'group' => t('Dibs'),
      'access callback' => 'dibs_rules_rules_access',
      'variables' => array(
        'context' => array(
          'label' => t('Context'),
          'type' => 'text',
          'description' => t('Some contextual information to aid debugging.'),
        ),
      ),
    ),
  );
}

//TODO:
function dibs_rules_rules_access() {
  return user_access('administer dibs transactions') && user_access('administer dibs settings');
}

function dibs_rules_rules_action_info() {
  return array(
    /*
    'dibs_create_single_order_line_transaction' => array(
      'label' => t('Create dibs transaction'),
      'parameter' => array(
        'quantity' => array(
          'type' => 'list<integer>', 'label' => t('Quantity'),
        )
        'currency' => '',
        'amount' => '',
      ),
    ),
    //'dibs_delete_transaction' => array(
    //  'label' => t('Delete dibs transaction'),
    //),
    'dibs_load_transaction' => array(
      ),
     */
    //TODO: Redirection parameter!
    'dibs_rules_load_transaction' => array(
      'label' => t('Load dibs transaction'),
      'group' => t('Dibs'),
      'parameter' => array(
        'tid' => array(
          'label' => t('Dibs transaction id'),
          'type' => 'integer',
          'optional' => FALSE,
        ),
      ),
      'provides' => array(
        'dibs_transaction' => array(
          'label' => t('Dibs transaction'),
          'type' => 'dibs_transaction',
          'save' => FALSE,
        ),
      ),
    ),
    /*
    'dibs_rules_load_transaction_by_order_id' => array(

      ),
    */ 
    'dibs_rules_create_transaction' => array(
      'label' => t('Create dibs transaction'),
      'group' => t('Dibs'),
      'parameter' => array(
        'order_id' => array(
          'label' => t('Order id'),
          'type' => 'text',
          'description' => t('Description'),
          'optional' => FALSE,
        ),
        'currency_code' => array(
          'label' => t('Currency code'),
          'type' => 'text',
          'description' => t('Description'),
          'options list' => 'dibs_currency_options_list',
          'optional' => TRUE,
        ),
        'redirect_url' => array(
          'label' => t('Redirect url'),
          'type' => 'uri',
          'description' => t('Description'),
          'optional' => TRUE, //Default dibs redirect page?
        ),
        //TODO: select list
        'api_module' => array(
          'label' => t('API module'),
          'type' => 'text',
          'descripiton' => t('Description'),
          'optional' => FALSE,
        ),
        //TODO: select list
        'api_delta' => array(
          'label' => t('API module'),
          'type' => 'text',
          'descripiton' => t('Description'),
          'optional' => FALSE,
        ),
        'quantity' => array(
          'label' => t('Quantity'),
          'type' => 'list<integer>',
          'description' => t('Description'),
          'optional' => TRUE,
        ),
        'description' => array(
          'label' => t('Description'),
          'type' => 'list<text>',
          'description' => t('Description'),
          'optional' => TRUE,
        ),
        'amount' => array(
          'label' => t('Amount'),
          'type' => 'list<integer>',
          'description' => t('Description'),
          'optional' => FALSE,
        ),
        'vat_amount' => array(
          'label' => t('VAT amount'),
          'type' => 'list<integer>',
          'description' => t('Description'),
          'optional' => TRUE,
        ),
        'vat_percent' => array(
          'label' => t('VAT percent'),
          'type' => 'list<integer>',
          'description' => t('Description'),
          'optional' => TRUE,
        ),
        'item_id' => array(
          'label' => t('Item id'),
          'type' => 'list<text>',
          'description' => t('Description'),
          'optional' => TRUE,
        ),
        'unit_code' => array(
          'label' => t('Unit code'),
          'type' => 'list<text>',
          'description' => t('Description'),
          'optional' => TRUE,
        ),
      ),
      'named parameter' => TRUE,
      'provides' => array(
        'dibs_transaction' => array(
          'label' => t('Dibs transaction'),
          'type' => 'dibs_transaction',
          'save' => TRUE,
        ),
      ),
    ),
  );
}

function dibs_rules_create_transaction($parameters) {
  // Order line parameters may be of length 1, x or empty where x is length of any one parameter not 1 or empty
  $order_line_parameters = array(
    'quantity' => DibsOrderInformation::QUANTITY,
    'description' => DibsOrderInformation::DESCRIPTION,
    'amount' => DibsOrderInformation::AMOUNT,
    'vat_amount' => DibsOrderInformation::VAT_AMOUNT,
    'vat_percent' => DibsOrderInformation::VAT_PERCENT,
    'item_id' => DibsOrderInformation::ITEM_ID,
    'unit_code' => DibsOrderInformation::UNIT_CODE,
  );

  $order_lines_count = 0;
  $order_information_types = array();
  
  $order_lines = array_intersect_key($parameters, $order_line_parameters);

   //Filter out empty columns
  $order_lines = array_filter($order_lines);
  $order_lines_counts = array_map('count', $order_lines);
  $order_lines_max = max($order_lines_counts);

  //Treat all columns with one row as default for that column
  if($order_lines_max > 1) {
    foreach(array_keys($order_lines_counts, 1) as $column) {
      if(in_array($column, array(DibsOrderInformation::QUANTITY, DibsOrderInformation::UNIT_CODE))) {
        $order_lines[$column] = array_fill(0, $order_lines_max, $order_lines[$column][0]);
      }
    }
    $invalid_order_lines = array_diff_key(
      $order_lines,
      array_flip(array_keys(array_map('count', $order_lines), $order_lines_max))
    );
    if(!empty($invalid_order_lines)) {
      $valid_parameter_keys = array(
        'order_id',
        'currency_code',
        'redirect_url',
        'api_module',
        'api_delta',
        'quantity',
        'description',
        'amount',
        'vat_amount',
        'vat_percent',
        'item_id',
        'unit_code',
      );

      $debug_info = array();

      $invalid_keys = array_keys(array_intersect_key(array_flip($order_line_parameters), $invalid_order_lines));
      $debug_info[] = 'Invalid order lines, ' . implode(', ', $invalid_keys) . ':';

      $valid_params = array_intersect_key($parameters, array_flip($valid_parameter_keys));
      $debug_info[] = print_r($valid_params, TRUE);

            $debug_message = implode("\n", $debug_info);
      rules_invoke_event('dibs_rules_invalid_order_information', $debug_message);
      throw new Exception($debug_message);
    }
  }

  $api_module = !empty($parameters['api_module']) ? $parameters['api_module'] : 'dibs_rules';
  $api_delta = !empty($parameters['api_delta']) ? $parameters['api_delta'] : 'dibs_rules';

  if(!empty($parameters['currency_code'])) {
    $currency_code = $parameters['currency_code'];
  }
  else {
    // Loading settings
    $settings = dibs_load_settings($api_module, $api_delta);
    $currency_code = $settings['general']['currency'];
  }

  $dibs_order_info = new DibsOrderInformation(
    array_values(array_intersect_key($order_line_parameters, $order_lines)), $currency_code
  );
  
  //Transpose array and add order data
  for($i = 0; $i < $order_lines_max; ++$i) {
    $row = array();
    foreach($order_lines as $column => $order_line) {
      $row[$order_line_parameters[$column]] = $order_line[$i];
    }
    //TODO: add option for this
    if(TRUE) {
      $currency = new Currency(dibs_currency_number_to_code($dibs_order_info->currency()));
      foreach(array(DibsOrderInformation::VAT_AMOUNT, DibsOrderInformation::AMOUNT) as $money_type) {
        if(isset($row[$money_type])) {
          $row[$money_type] = (int) ($currency->subunitToUnit * $row[$money_type]);
        }
      }

      if(isset($row[DibsOrderInformation::VAT_PERCENT])) {
        $row[DibsOrderInformation::VAT_PERCENT] = $row[DibsOrderInformation::VAT_PERCENT] * 100;
      }
    }
    $dibs_order_info->addRowAssoc($row);
  }

  $data = array(
    'api_module'          => $api_module,
    'api_delta'           => $api_delta,
    'payment_price'       => $dibs_order_info->total_amount(), //TODO
    'payment_order_id'    => $parameters['order_id'],
    'params'              => array(
      'dibs_rules_order_id' => $parameters['order_id'], //??
    ),
  );

  $data['order_information'] = $dibs_order_info;
  $hash = dibs_payment_prepare($data, FALSE);

  $transaction = (object) dibs_transaction_load($hash, 'hash');

  return array(
    'dibs_transaction' => $transaction,
  );

}

function dibs_rules_load_transaction($tid) {
  $transaction =  (object) dibs_transaction_load($tid);
  return array(
    'dibs_transaction' => $transaction
  );
}

function dibs_rules_rules_data_info() {
  return array(
    'dibs_transaction' => array(
      'label' => t('Dibs transaction'),
      'wrap' => TRUE,
      'wrapper class' => 'DibsTransactionRulesDataWrapper',
      'token type' => 'dibs_transaction',
      'property info' => array(
        'tid' => array(
          'label' => t('Dibs transaction id'),
          'type' => 'integer',
          //'schema field' => 'tid',
        ),
      /*
      'order_information' => array(
        'label' => t('Order information'),
        'type' => 'list<struct>',
        'property info' => array(
        ),
      ),
       */
        'payment_order_id' => array(
          'label' => t('Order id'),
          'type' => 'text',
          //'getter callback' => 'dibs_rules_payment_property_get',
        ),
        'payment_order_id_numeric' => array(
          'label' => t('Order id'),
          'type' => 'integer',
          'getter callback' => 'dibs_rules_payment_property_get_numeric',
        ),
        'payment_redirect_url' => array(
          'label' => t('Payment redirect url'),
          'type' => 'uri',
          'getter callback' => 'dibs_rules_payment_redirect_url_get',
        ),
        'order_information_text' => array(
          'label' => t('Order information'),
          'type' => 'text',
          'getter callback' => 'dibs_rules_order_information_text_get',
        ),
        'order_information_html' => array(
          'label' => t('Order information'),
          'type' => 'text',
          'getter callback' => 'dibs_rules_order_information_html_get',
        ),
        'order_information_total_amount_formatted' => array(
          'label' => t('Total amount (formatted)'),
          'type' => 'text',
          'getter callback' => 'dibs_rules_order_information_total_amount_formatted_get',
        ),
      ),
    ),
  );
}

//TODO: implement savable!
class DibsTransactionRulesDataWrapper extends RulesIdentifiableDataWrapper implements RulesDataWrapperSavableInterface {

  protected function extractIdentifier($transaction) {
    return $transaction->tid;
  }

  protected function load($tid) {
    return (object) dibs_transaction_load($tid);
  }

  public function __construct($type, $data = NULL, $info = array()) {
    parent::__construct($type, $data, $info);
  }

  public function save() {

  }

  public function info() {
    $info = parent::info();
    return $info;
  }

}

function dibs_rules_payment_property_get_numeric($data, array $options, $name, $type, $info) {
  return intval($data->{substr($name, 0, -8)});
}

function dibs_rules_payment_property_get($data, array $options, $name, $type, $info) {
  return $data->{'payment_' . $name};
}

function dibs_rules_order_information_text_get($data, array $options, $name, $type, $info) {
  if(!empty($data->order_information)) {
    $order_info = unserialize($data->order_information);
    return theme('dibs_order_information_text', array(
      'order_information' => $order_info,
    ));
  }
}

//Should provide separate wrapper class for order information? but no time for that now
function dibs_rules_order_information_total_amount_formatted_get($data, array $options, $name, $type, $info) {
  if(!empty($data->order_information)) {
    $order_info = unserialize($data->order_information);
    $currency = new Currency(dibs_currency_number_to_code($order_info->currency()));
    $amount = new Money($order_info->total_amount(), $currency);
    return $amount->format();
  }
}

function dibs_rules_order_information_html_get($data, array $options, $name, $type, $info) {
  //TODO: call template
  return 'order information html';
}

function dibs_rules_payment_redirect_url_get($data, array $options, $name, $type, $info) {
  return url('payment/dibs/redirect/' . $data->api_hash, array('absolute' => TRUE));
}
