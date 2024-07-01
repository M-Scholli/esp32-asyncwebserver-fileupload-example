const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
</head>
<body>
  <p><h1>File Upload</h1></p>
  <p>Free Storage: %FREEFATFS% | Used Storage: %USEDFATFS% | Total Storage: %TOTALFATFS%</p>
  <form method="POST" id="upload" action="/upload" enctype="multipart/form-data"><input type="file" name="data"/><input type="submit" name="upload" value="Upload" title="Upload File"></form>
  <br />
  <label for="file">Uploading progress:</label>
  <progress id="progressUpload" value="0" max="100"></progress>
  <p>After clicking upload it will take some time for the file to firstly upload and then be written to FatFS. Please be patient.</p>
  <p>Once uploaded the page will refresh and the newly uploaded file will appear in the file list.</p>
  <p>If a file does not appear, it will be because the file was too big, or had unusual characters in the file name (like spaces).</p>
  <p>You can see the progress of the upload by watching the serial output.</p>
  <p>%FILELIST%</p>
  <script>
  const fileEle = document.getElementById('upload');
  var fileSizeTotal = 0;
  var fileSize = 0;

  fileEle.addEventListener('change', function (e) {
      const files = e.target.files;
      if (files.length !== 0) {
          fileSizeTotal = `${files[0].size}`;
      }
  });

  setInterval(function() {
    // Call a function repetatively with 1 Second interval
    getData();
  }, 1000); //1000mSeconds update rate

  function getData() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        fileSize = this.responseText;
        if (fileSizeTotal > 0) {
          var percent = 0.0;
          if (fileSize > 0) {
            percent = (fileSize / fileSizeTotal) * 100;
          } else {
            percent = 0;
          }
          if (percent >= 99.9) {
            percent = 100;
            document.getElementById("progressUpload").value =
            100;
          } else {
            document.getElementById("progressUpload").value =
            (percent).toFixed(1);
          }
        }
      }
    };
    xhttp.open("GET", "readProgress", true);
    xhttp.send();
  }
  </script>
</body>
</html>
)rawliteral";
