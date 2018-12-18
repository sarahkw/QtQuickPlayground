import QtQuick 2.10
import QtQuick.Window 2.3
import QtQuick.Controls 2.2

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")

    Column {
        spacing: 10

        Button {
            text: "Gc"
            onClicked: {
                gc()
            }
        }
        Button {
            text: "Direct"
            onClicked: {
                console.info("DIRECT", control.delay())
            }
        }

        Button {
            text: "Async"
            onClicked: {
                control.async_delay(function (r) {
                    console.info('ASYNC', r)
                })
            }
        }

        Row {
            TextField {
                id: txtSeconds
            }
            Button {
                text: "Sleep"
                onClicked: {
                    control.async_sleep(parseInt(txtSeconds.text, 10),
                            function (r) {
                                console.info("ASYNC SLEEP RETURNED", r)
                            })
                }
            }
        }

        Row {
            spacing: 10
            Button {
                text: "mobject callback"
                onClicked: {
                    control.mobjectCallback(function (f) {
                        console.info(f)
                        console.info(f.muhName())
                    })
                }
            }
            Button {
                text: "mobject signal"
                onClicked: {
                    control.mobjectReply.connect(function (m) {
                        console.info(m)
                        console.info(m.muhName())
                    });
                    control.mobjectSignal()
                }
            }
            Button {
                text: "mobject async"
                onClicked: {
                    control.async_mobjectDirect(function (f) {
                        console.info(f)
                        console.info(f.muhName())
                    })
                }
            }
            Button {
                text: "mobject async bad callback"
                onClicked: {
                    control.async_mobjectDirect(function (f) {
                        f.doesntExist()
                    })
                }
            }
        }

        Button {
            text: "multiReturn"
            onClicked: {
                control.async_multiReturn(function (a, b, c, d) {
                    console.info("a is", a, "b is", b, "c is", c)
                    console.info(b.muhName())
                    console.info("d is", d)
                })
            }
        }

        Text {
            text: "hello"
            font { bold: true; italic: true; pixelSize: 20; capitalization: Font.AllUppercase }
        }
    }
}
