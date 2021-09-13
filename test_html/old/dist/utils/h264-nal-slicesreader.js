'use strict';

Object.defineProperty(exports, "__esModule", {
    value: true
});

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

var _events = require('../events');

var _events2 = _interopRequireDefault(_events);

var _eventHandler = require('../event-handler');

var _eventHandler2 = _interopRequireDefault(_eventHandler);

var _h264Demuxer = require('../demux/h264-demuxer');

var _h264Demuxer2 = _interopRequireDefault(_h264Demuxer);

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { default: obj }; }

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; } /*
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                * H264 NAL Slicer
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                */


var SlicesReader = function (_EventHandler) {
    _inherits(SlicesReader, _EventHandler);

    function SlicesReader(wfs) {
        var config = arguments.length > 1 && arguments[1] !== undefined ? arguments[1] : null;

        _classCallCheck(this, SlicesReader);

        var _this = _possibleConstructorReturn(this, (SlicesReader.__proto__ || Object.getPrototypeOf(SlicesReader)).call(this, wfs, _events2.default.H264_DATA_PARSING));

        _this.config = _this.wfs.config || config;
        _this.h264Demuxer = new _h264Demuxer2.default(wfs);
        _this.wfs = wfs;
        _this.lastBuf = null;
        _this.nals = [];
        return _this;
    }

    _createClass(SlicesReader, [{
        key: 'destroy',
        value: function destroy() {
            this.lastBuf = null;
            this.nals = [];
            _eventHandler2.default.prototype.destroy.call(this);
        }
    }, {
        key: '_read',
        value: function _read(buffer) {
            var typedAr = null;
            this.nals = [];
            if (!buffer || buffer.byteLength < 1) return;
            if (this.lastBuf) {
                typedAr = new Uint8Array(buffer.byteLength + this.lastBuf.length);
                typedAr.set(this.lastBuf);
                typedAr.set(new Uint8Array(buffer), this.lastBuf.length);
            } else {
                typedAr = new Uint8Array(buffer);
            }
            var lastNalEndPos = 0;
            var b1 = -1; // byte before one
            var b2 = -2; // byte before two
            var nalStartPos = new Array();
            for (var i = 0; i < typedAr.length; i += 2) {
                var b_0 = typedAr[i];
                var b_1 = typedAr[i + 1];
                if (b1 == 0 && b_0 == 0 && b_1 == 0) {
                    nalStartPos.push(i - 1);
                } else if (b_1 == 1 && b_0 == 0 && b1 == 0 && b2 == 0) {
                    nalStartPos.push(i - 2);
                }
                b2 = b_0;
                b1 = b_1;
            }
            if (nalStartPos.length > 1) {
                for (var i = 0; i < nalStartPos.length - 1; ++i) {
                    this.nals.push(typedAr.subarray(nalStartPos[i], nalStartPos[i + 1] + 1));
                    lastNalEndPos = nalStartPos[i + 1];
                }
            } else {
                lastNalEndPos = nalStartPos[0];
            }
            if (lastNalEndPos != 0 && lastNalEndPos < typedAr.length) {
                this.lastBuf = typedAr.subarray(lastNalEndPos);
            } else {
                if (!!!this.lastBuf) {
                    this.lastBuf = typedAr;
                }
                var _newBuf = new Uint8Array(this.lastBuf.length + buffer.byteLength);
                _newBuf.set(this.lastBuf);
                _newBuf.set(new Uint8Array(buffer), this.lastBuf.length);
                this.lastBuf = _newBuf;
            }
        }
    }, {
        key: 'onH264DataParsing',
        value: function onH264DataParsing(event) {
            this._read(event.data);
            var $this = this;
            this.nals.forEach(function (nal) {
                $this.wfs.trigger(_events2.default.H264_DATA_PARSED, {
                    data: nal
                });
            });
        }
    }]);

    return SlicesReader;
}(_eventHandler2.default);

exports.default = SlicesReader;