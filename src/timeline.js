Pebble.addEventListener('ready', function() {
    if(!localStorage.subscribed) {
        Pebble.timelineSubscribe('cny', 
          function () { 
            console.log('Subscribed to cny');
            localStorage.subscribed = true;
          }, 
          function (errorString) { 
            console.log('Error subscribing to topic cny: ' + errorString);
          }
        );
    }
});