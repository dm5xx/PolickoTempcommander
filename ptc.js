var currentJson;

$(document).ready(function() {
    init();
 });
 
 var init = function()
 { 
    reloadData();
    window.setInterval(function(){ reloadData(); console.log("Reloaded...");}, 60000);
 } 

function getContent() {
    $.ajax({
        crossDomain: true,
        async : false,
        type: "GET",
        headers: {
            'Access-Control-Allow-Origin': '*'
        },
        url: configAddress + "/Get",
        dataType: 'jsonp',
        jsonpCallback:'myCB'})
        .done(function(data){ 
            dataArray = data.v.split('|');
    });
}

function setContent(d, v) {
	return $.ajax({
        url: configAddress +"/Set/"+d+"/"+v,
		dataType: 'text',
		success: function(string) {
            reloadData();
        }	
	});
}

function reloadData()
{
    getContent();
    window.setTimeout(fillContent, 500);
    
    var currentdate = new Date();
    var datetime = currentdate.getHours() + ":" +currentdate.getMinutes() + ":" + currentdate.getSeconds();    
    $('#re').html(datetime);
}

function fillContent() {

    for(var a=1; a<8; a++)
    {
        $('#s'+a).html(dataArray[a-1]);
    }

    if(dataArray[7] == 1)
    {
        $('#mode').html('MAN');
        $('#mode').css('background', 'red');
    }
    else
    {
        $('#mode').html('AUTO');
        $('#mode').css('background', 'green');
    }

    if(dataArray[8] == 1)
    {
        $('#relay1').html('ON');
        $('#relay1').css('background', 'red');
    }
    else
    {
        $('#relay1').html('OFF');
        $('#relay1').css('background', 'green');
    }

    if(dataArray[9] == 1)
    {
        $('#relay2').html('ON');
        $('#relay2').css('background', 'red');
    }
    else
    {
        $('#relay2').html('OFF');
        $('#relay2').css('background', 'green');
    }    
}

function switchIt(d, v)
{
    switch(d) {
        case 0:
          // mode
          v == 0 ? setContent(9, 0) : setContent(9, 1);
          break;
        case 1:
          // relay 1
          v == 0 ? setContent(1, 0) : setContent(1, 1);
          break;
        case 2:
          // relay 2
          v == 0 ? setContent(2, 0) : setContent(2, 1);
          break;
    };
}