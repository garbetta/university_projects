$("requestForm").submit($.get("http://localhost:8888/search", {title: $("#title").val(), author: $("#author").val()}, function(data){
    if (data){
        var wind = windows.open("../result.html");
        for(var i = 0; i < data.length; i++){
            wind.$("section").append("<img src=" + data[i] + ">");
        };
    };
}));

function test(){
    alert("title:" + $("#title").val());
    alert("author:" + $("#author").val());
    $.get("http://localhost:8888/search", {title: $("#title").val(), author: $("#author").val()})
    .done(function(data){
        alert("callback:" + data);
        if (data){
            alert(data);
            var wind = windows.open("../result.html");
            for(var i = 0; i < data.length; i++){
                wind.$("section").append("<img src=" + data[i] + ">");
            };
        }});
    return false;
}