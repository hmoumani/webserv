<?php

session_start();

for ($i = 0; $i <= 100; $i++)
{
    echo $_SESSION['name' . $i] . "<br>";
}