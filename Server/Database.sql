SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET AUTOCOMMIT = 0;
START TRANSACTION;
SET time_zone = "+00:00";
CREATE DATABASE IF NOT EXISTS `MegaLAN` DEFAULT CHARACTER SET utf8 COLLATE utf8_bin;
USE `MegaLAN`;

CREATE TABLE `Accounts` (
  `Username` varchar(100) COLLATE utf8_bin NOT NULL,
  `UserHash` varchar(40) COLLATE utf8_bin NOT NULL,
  `IsAdmin` tinyint(1) NOT NULL,
  `Email` mediumtext COLLATE utf8_bin NOT NULL,
  `PasswordHash` varchar(40) COLLATE utf8_bin DEFAULT NULL,
  `CryptoHash` varchar(64) COLLATE utf8_bin DEFAULT NULL,
  `VerifyKey` varchar(16) COLLATE utf8_bin DEFAULT NULL,
  `VerifyTime` int(11) NOT NULL,
  `Token` varchar(20) COLLATE utf8_bin DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

CREATE TABLE `IP_Allocations` (
  `VLANID` varchar(40) COLLATE utf8_bin NOT NULL,
  `MAC` varchar(12) COLLATE utf8_bin NOT NULL,
  `IPv4` mediumtext COLLATE utf8_bin NOT NULL,
  `IPv6` mediumtext COLLATE utf8_bin NOT NULL,
  `Time` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

CREATE TABLE `VLANs` (
  `ROWID` bigint(20) NOT NULL,
  `VLANID` varchar(40) COLLATE utf8_bin NOT NULL,
  `Public` tinyint(1) NOT NULL,
  `Owner` varchar(40) COLLATE utf8_bin DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

CREATE TABLE `VLAN_Info` (
  `VLANID` varchar(40) COLLATE utf8_bin NOT NULL,
  `Name` varchar(150) COLLATE utf8_bin NOT NULL,
  `Description` longtext COLLATE utf8_bin NOT NULL,
  `IPv4` longtext COLLATE utf8_bin DEFAULT NULL,
  `IPv6` longtext COLLATE utf8_bin DEFAULT NULL,
  `CryptoKey` varchar(64) COLLATE utf8_bin NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin ROW_FORMAT=COMPACT;

CREATE TABLE `VLAN_Membership` (
  `VLANID` varchar(40) COLLATE utf8_bin NOT NULL,
  `UserID` varchar(40) COLLATE utf8_bin NOT NULL,
  `MAC` varchar(12) COLLATE utf8_bin NOT NULL,
  `IP` mediumtext COLLATE utf8_bin NOT NULL,
  `Port` mediumint(9) NOT NULL,
  `CareOf` mediumtext COLLATE utf8_bin DEFAULT NULL,
  `Time` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;


ALTER TABLE `Accounts`
  ADD UNIQUE KEY `UserHash` (`UserHash`),
  ADD UNIQUE KEY `Username` (`Username`),
  ADD KEY `VerifyKey` (`VerifyKey`),
  ADD KEY `Token` (`Token`);

ALTER TABLE `IP_Allocations`
  ADD UNIQUE KEY `PEERID` (`VLANID`,`MAC`),
  ADD KEY `VLANID` (`VLANID`),
  ADD KEY `Time` (`Time`);

ALTER TABLE `VLANs`
  ADD PRIMARY KEY (`ROWID`),
  ADD UNIQUE KEY `VLANID` (`VLANID`),
  ADD KEY `Owner` (`Owner`);

ALTER TABLE `VLAN_Info`
  ADD KEY `VLANID` (`VLANID`,`Name`);

ALTER TABLE `VLAN_Membership`
  ADD UNIQUE KEY `members_idx` (`VLANID`,`UserID`,`MAC`,`IP`(100),`Port`),
  ADD KEY `UserID` (`UserID`);


ALTER TABLE `VLANs`
  MODIFY `ROWID` bigint(20) NOT NULL AUTO_INCREMENT;


ALTER TABLE `IP_Allocations`
  ADD CONSTRAINT `IP_Allocations_ibfk_1` FOREIGN KEY (`VLANID`) REFERENCES `VLANs` (`VLANID`);

ALTER TABLE `VLANs`
  ADD CONSTRAINT `VLANs_ibfk_1` FOREIGN KEY (`Owner`) REFERENCES `Accounts` (`UserHash`);

ALTER TABLE `VLAN_Info`
  ADD CONSTRAINT `VLAN_Info_ibfk_1` FOREIGN KEY (`VLANID`) REFERENCES `VLANs` (`VLANID`);

ALTER TABLE `VLAN_Membership`
  ADD CONSTRAINT `VLAN_Membership_ibfk_1` FOREIGN KEY (`VLANID`) REFERENCES `VLANs` (`VLANID`),
  ADD CONSTRAINT `VLAN_Membership_ibfk_2` FOREIGN KEY (`UserID`) REFERENCES `Accounts` (`UserHash`);
COMMIT;
